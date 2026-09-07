<!-- Note (2026-09-07): Galapix SQLite `TileGenerationJob` path removed; file tiles use thumtoo. Overview still uses TileGenerator::load_surface. -->

<!--
SPDX-FileCopyrightText: 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Tile loading strategy

How Galapix should load tiles so the **viewport stays responsive**: show what
you are looking at first, allow texture pop-in, never wait for a whole archive
or full pyramid before painting something useful.

This compares **master** (classic SQLite + in-process generation), **develop**
with the legacy SQLite path, and **develop + thumtoo**, then proposes a target
policy.

## Goals

1. **First paint fast** — something on screen for the current view (even blurry).
2. **Viewport priority** — visible tiles at the current scale beat everything else.
3. **Background fill** — other scales / off-screen tiles / full pyramids later.
4. **No frame stalls** — decode and I/O off the GUI thread; budgeted GL uploads
   (see recent develop work).
5. **Cancel freely** — when the user zooms or pans away, drop obsolete work.

Pop-in and temporary lower-resolution stand-ins are acceptable. Waiting for a
full archive extract or a complete multi-scale cut is not.

---


## Pipeline (working model — not settled)

Open questions: whether **size/orientation** should be a separate pass from
**fast overview**, and how to live with **libarchive’s weak random access**.
Benchmark before locking the design.

### Requirements that are settled

1. **Interactive path must not full-pyramid-batch** on a single tile miss.
2. **Missing tile → next coarser tile** (then placeholder), never a blank wait
   for the whole image.
3. **Fast overview ≠ grid tiles.** Low-quality JPEG/EXIF previews must **not**
   overwrite or share identity with high-quality 256² tiles (separate store /
   key / quality flag). Upgrade is replace-on-arrival, not mutate-in-place of
   the same blob as “the” tile.
4. **GUI never blocks** on decode/hash/archive extract.

### Size / orientation vs fast overview

| Approach | Pros | Cons |
|----------|------|------|
| **Split** | Smallest work for layout-only; can show empty frames early | Extra open/seek for archives (painful with libarchive) |
| **Merged** | One archive member read → size + soft preview | Heavier minimum work; still need quality separation from real tiles |

For **plain files**, split is often fine (header-only size is cheap).  
For **archive members**, seeking is expensive — **merging size + overview into
one sequential read** of the member may win. **Measure** (time to first size,
time to first pixels, seeks per member, cold vs warm).

### Fast overview quality isolation

- libjpeg scale / EXIF thumb: **low quality**, good for “something on screen”.
- Pyramid tiles: **high quality** (e.g. JPEG q≈80 grid cells).
- Store overview under a distinct key (e.g. `overview` / `preview` max-edge,
  or a non-tile layer), **not** as `tiles(scale,x,y)` at the same coordinates
  as real cells. Otherwise a soft preview can be mistaken for a finished tile
  and never upgraded, or pollute the durable tile cache.

Viewer: draw overview only as stand-in; when a real tile arrives, drop the
overview for that region.

### Grid tiles: on demand (default)

- One miss → generate **that cell** (or at most that scale’s visible set).
- **No** automatic `min_scale…computed_max` pyramid on interactive request.
- Explicit batch remains **`thumtoo-prepare`** / idle background only.

### Archives and libarchive seeking

Random access inside many archive formats is **slow** (solid compression,
weak seek, re-decompress from earlier points). Implications:

- Prefer **one sequential pass per member** when touching an archive entry
  (size + overview, or a small set of tiles), over many tiny seeks.
- **Auto-batch on the thumtoo side** when several tile requests share the same
  archive member (or same content id) within a short window: open once, satisfy
  many cells, close. That is **request coalescing**, not “build the whole
  pyramid before returning anything”.
- Coalesce carefully: still return the **first needed** tile as soon as it
  exists; do not hold the UI until the whole batch finishes.
- Benchmark candidates: zip store vs deflate, 7z solid vs non-solid, tar.gz,
  rar; cold page cache vs warm.

### Fallback when a tile is missing

```text
requested high-quality tile (scale, x, y)
    → real tile if present
    → else coarser real tile (find_smaller_tile)
    → else overview / fast-jpeg stand-in (if any)
    → else placeholder colour
```

### Mapping to current code

| Concern | Today | Direction |
|---------|--------|-----------|
| Size at open | Batched thumtoo probe | Keep; consider merge with overview for archives |
| Fast overview | Not on pure thumtoo path | Add; **separate** from tile table |
| Pyramid on miss | `request_tile` often full range | **Remove** for interactive path |
| Archive multi-tile | Likely repeated open/seek | thumtoo **coalesce** same-member requests |
| Lower-res fallback | `find_smaller_tile` | Keep; overview as extra tier |

## What master did well

### On-demand, per tile

`DatabaseThread::request_tile` (master):

1. Look up **one** tile in the SQLite tile DB.
2. If present → return it immediately.
3. If missing → `generate_tile` for **that** `(scale, x, y)` (possibly sharing a
   running `TileGenerationJob` for the same file).

Generation is driven by **viewer demand**, not by “prepare the whole image”.

### Progressive delivery during generation

`TileGenerationJob` collects late requests while running and can fire callbacks
as individual tiles are produced (`process_tile` / tile list). The viewer does
not need the entire job to finish before drawing the first tile.

### JPEG decode scale (libjpeg)

`TileGenerator::load_surface` for JPEG:

- Uses **libjpeg `scale_num` / `scale_denom`** with factors **1, 2, 4, 8**.
- Chooses `jpeg_scale = min(2^min_scale, 8)` so coarse levels often **decode a
  downscaled scan** without reading a full-resolution raster first.

That is the “load a thumbnail straight from the .jpg” behaviour: DCT-domain
downscale, not a separate EXIF thumbnail stream (though EXIF thumbs could be a
further fast path). It makes overview / high `tilescale` cheap.

### While waiting: lower-res stand-ins

`ImageRenderer` / `ImageTileCache::find_smaller_tile` draws a coarser tile
stretched into the cell (or a placeholder colour) until the requested tile
arrives. Zooming does not stay blank.

### Cancel out-of-view work

Each draw computes the visible tile rect and `cancel_jobs` for requests outside
that rect / scale so the queue tracks the camera.

### Gaps on master

- Queue was FIFO with a busy-wait; comments already wanted a **priority queue**.
- Full-image generation for a scale range (`request_tiles` / `prepare`) still
  batch-oriented by design.
- No separate “embedded EXIF thumbnail first” path beyond libjpeg scale.

---

## What develop + thumtoo does today

### Path

`ThumtooTileProvider::request_tile` → `Client::request_tile` → worker
`handle_ensure_tiles` → `build_tile_pyramid` / `cut_pyramid_from_vips`.

### Batch bias

For a **single** tile request, thumtoo currently sets roughly:

- `min_scale = requested scale`
- `max_scale = -1` → treated as **computed max** (until the image fits in one
  256² tile)

So one visible-cell miss can trigger **building many scales / many tiles** for
that content before the one cell you need is stored and returned. That feels
like “batch”: nothing useful until a large unit of work finishes.

Archives amplify this: extract / open / pyramid work may complete as one unit
before any member tile is returned to Galapix.

### Decode path

thumtoo uses **vips** (full pipeline), not libjpeg’s 1/2/4/8 load scale. Coarse
tiles still require a shrink chain from a full (or ladder) decode, not a cheap
DCT thumbnail pass over the original JPEG.

### Galapix-side improvements already on develop

- JPEG decode off the GUI thread (inline worker executor).
- Bounded GL uploads per frame.
- `find_smaller_tile` still used when a cell is missing.
- `cancel_jobs` still runs from `ImageRenderer`.
- max_scale aligned with thumtoo’s single-tile coverage.

These fix **stutter**; they do not fix **latency to first useful tile** when
thumtoo batches a whole pyramid or archive.

### Legacy SQLite path on develop

Still closer to master: `DatabaseThread::request_tile` → generate one tile /
shared job, and `TileGenerator` still has the **JPEG scale** load path. Prefer
this behaviour as the reference for “responsive” semantics even when the store
is thumtoo.

---

## Target strategy

### Priority order (highest first)

| Priority | What | Why |
|----------|------|-----|
| P0 | **Visible** tiles at **current** `tilescale` | What the user sees now |
| P1 | **One step coarser** than current (stand-in / quick overview) | Instant soft image while P0 loads |
| P2 | **One step finer** (prefetch when zoom trend is in) | Optional |
| P3 | Rest of current scale (off-screen margin) | Pan headroom |
| P4 | Full pyramid / other scales / archive siblings | Background only |

Never block P0 on P4.

### Work units

| Kind | When | Notes |
|------|------|--------|
| **Individual tile** | Cache miss for a visible cell | Encode/store **that** `(scale,x,y)` first; return ASAP |
| **Single-scale strip** | Optional | All tiles at one scale for the visible rect only |
| **Pyramid batch** | Idle / explicit prepare | `thumtoo-prepare` or low-priority background |
| **Archive member** | Open one image | Process **that** member for P0 tiles; do not require whole archive ladder first |

### Fast sources for coarse pixels

1. **Already in cache** (thumtoo DB or SQLite) — cheapest.
2. **libjpeg scaled decode** (1/2/4/8) from the original file when the URL is a
   filesystem JPEG — ideal for high `tilescale` / overview without vips.
3. **Embedded EXIF/JPEG thumbnail** (if present and large enough) — optional
   ultra-fast first frame; may be cropped or low quality; replace when real
   tiles arrive.
4. **vips / full decode** — authoritative tiles; run after or in parallel once
   a quick stand-in is on screen.

Galapix can keep using `find_smaller_tile` so (2)/(3) only need to feed **some**
coarse tile into the cache.

### thumtoo API expectations (for upstream)

Prefer:

- `request_tile(uri, scale, x, y)` builds **at most that scale** (or only that
  cell), not `min_scale…computed_max` by default.
- Optional `request_tile_pyramid(uri, min, max)` or prepare CLI for batch.
- Optional “overview” helper: return a single downscaled image via JPEG scale
  or EXIF thumb without cutting the full grid.
- Archive: open/extract **member** for the requested URI; do not serialize all
  members behind one UI-facing tile.

Until API changes land, Galapix may:

- Prefer requesting a **coarse scale first** (one tile often covers the image),
  draw it, then request the true visible scale; or
- Call a future “single cell only” flag if added.

### Galapix viewer policy

1. On scale change: cancel non-visible jobs; request **visible** cells at the
   new scale (P0).
2. Optionally request **parent** coarse tile(s) first if not in cache (P1).
3. Cap concurrent generation jobs per image and globally; prefer visible set.
4. Keep upload budget on the main thread; allow pop-in.
5. Do **not** wait for `prepare` / full pyramid for interactive view
   (`thumtoo-prepare` remains the offline batch tool).

### What not to do

- Do not require a full archive scan before showing the first image.
- Do not treat “first tile miss” as “build entire pyramid”.
- Do not decode or upload unbounded work on the GUI thread.
- Do not drop stand-in rendering while waiting for authoritative tiles.

---

## Implementation sketch (ordered)

1. **thumtoo**: single-tile / single-scale generation path for `request_tile`
   (biggest win for “batch” feel).
2. **thumtoo or Galapix**: JPEG libjpeg-scale (or EXIF thumb) overview fast path
   for local `.jpg` before full vips pyramid.
3. **Galapix**: optional explicit P1 request (coarser tile) when P0 misses.
4. **Priority queue** for generation (visible > prefetch > background) —
   longstanding master FIXME.
5. **Archive**: per-member readiness for the open URI only.
6. Metrics (debug): time-to-first-tile, tiles completed per frame, cancel rate.

---

## Mapping to commands

| User intent | Tool |
|-------------|------|
| Browse / zoom now | `galapix` + on-demand tiles (this strategy) |
| Fill disk cache offline | `thumtoo-prepare` (batch pyramid OK) |
| Legacy full Galapix prepare | Not ported; use thumtoo prepare |

---

## Related docs

- [THUMTOO.md](THUMTOO.md) — flags, pure mode, scale range
- [DEVELOP_VS_MASTER.md](DEVELOP_VS_MASTER.md) — branch gap / UI focus
- [DEPENDENCIES.md](DEPENDENCIES.md) — what thumtoo replaces


## Startup: "Processing URLs" slowness

The progress line during `view` is **not** primarily Galapix SHA1 checksums on
the open path. With thumtoo it was dominated by **per-file size probes**:

```text
for each URL:
  ThumtooTileProvider::create
    → request_size + Client::drain()   // open file, optional content id / decode
```

That is **O(n) sequential** heavy work before the viewer window appears.

### Fix (develop)

1. **Batch** `request_size` for all URLs missing size, then **one** `drain()`.
2. Build providers with `create_from_size` from the cache (no second probe).
3. Single-file / miss still uses `create()` (one probe) as fallback.

thumtoo may still hash or decode inside each size job; batching overlaps that
work on Client worker threads instead of serializing drain boundaries.

### Future

- Defer size until first layout / first visible image (open viewer immediately).
- Header-only JPEG size (SOF) without full content-id hash when acceptable.
- Do not SHA1 whole files on Galapix open path for pure thumtoo view.


## Implementation status (Galapix develop)

| Item | Status |
|------|--------|
| No interactive full-pyramid on tile miss | **Upstream thumtoo** (`tile_max_scale = scale` on `request_tile`; still may encode all cells at that scale) |
| `request_tile_pyramid` / prepare for batch | Unchanged (explicit batch OK) |
| Missing tile → coarser tile | Present (`find_smaller_tile` draw path) |
| Request coarser stand-in on miss | **Done** — queue max_scale overview + one parent cell before target; `cancel_jobs` keeps coarser REQUESTED jobs |
| Fast overview ≠ tile store | **In-memory overview** via `ImageOverview` (libjpeg scale, not tile identity). EXIF thumb + disk cache still optional |
| libjpeg overview (stdio) | **Done** — `ImageOverview` / `OverviewLoadJob` |
| EXIF embedded thumb overview | **Not done** (optional) |
| Archive same-member coalesce | **Not done** (thumtoo) |
| Single **cell** only (not full scale grid) | **Not done** (thumtoo cut API) |
| Drop Galapix SQLite tile DB | Pure thumtoo view already skips `cache4_tiles`; resource DB remains for patterns / metadata |
| Console tile backlog stats (`l`) | **Done** — pending requests/uploads/cache entries + thumtoo callback queue (see [STATUS_REPORTING.md](STATUS_REPORTING.md)) |
| On-screen status HUD | **Not done** — blocked on text rendering library choice |

### Feature gaps vs old Galapix tile cache (spirit)

| Old Galapix | thumtoo (+ patch) | Gap? |
|-------------|-------------------|------|
| Per-scale generation with progressive jobs | Single-scale on demand | Smaller gap after patch |
| libjpeg 1/2/4/8 overview | — | **Yes** — overview path |
| SQLite tile identity (SHA1) | sha256 content id | Fine (improved) |
| Archive browse via arxp | libarchive in thumtoo | Coalesce / seek still weak |
| `prepare` CLI | `thumtoo-prepare` | Policy: use thumtoo |

Galapix legacy **tile** tables can stay unused in pure thumtoo mode; do not invest in new SQLite tile features.

## Session handoff (2026-09-07)

Upstream thumtoo now: multi-worker queue, archive member cache, tile-job
coalesce, prepare `--stats` (wall vs cpu-sum). Galapix interactive path still
benefits most from single-scale `request_tile` + `ImageOverview` + stand-in
parents. Remaining gap: **single-cell** cut (whole scale grid still possible
inside one scale).
