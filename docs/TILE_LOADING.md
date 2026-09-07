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


## Canonical pipeline (target)

Three kinds of work, in order. Later steps must not block earlier paint.

### Pass 1 — Size / orientation (cheap)

- Header-only or thumtoo `request_size` (batched, one drain).
- Enough to place images on the workspace and pick `tilescale`.
- **No** full decode, **no** full-file hash on the critical path if avoidable
  (provisional content id until hash is needed).

### Pass 2 — Fast overview (optional but important for JPEG)

- Prefer **libjpeg scaled decode** (1/2/4/8) and/or embedded EXIF thumbnail.
- Produce **one coarse stand-in** (often a single tile at high scale, or a
  soft full-image texture) so the user sees something immediately.
- Still off the GUI thread; upload with the per-frame budget.

### Pass 3 — Grid tiles (authoritative)

Two modes, both valid:

| Mode | When | Behaviour |
|------|------|-----------|
| **On demand** | Interactive `view` | Generate only **visible** `(scale,x,y)` (or that scale); return ASAP |
| **Batch** | Idle / `thumtoo-prepare` | Full pyramid or archive set in the background |

Never make interactive view wait for a full-pyramid batch.

### Fallback when a tile is missing

```text
requested tile (scale, x, y)
    → if present: draw it
    → else: find_smaller_tile / parent scales (stretch)
    → else: placeholder (e.g. solid colour)
```

`ImageRenderer` / `ImageTileCache::find_smaller_tile` already implement the
“next lower res” idea on develop; keep that path strong while Pass 3 fills in.

### Mapping to current code

| Step | Today | Gap |
|------|--------|-----|
| Pass 1 | Batched thumtoo size probe at open | Defer until after window open; header-only JPEG |
| Pass 2 | Missing for pure thumtoo (vips path only) | libjpeg scale / EXIF overview into cache |
| Pass 3 on demand | thumtoo `request_tile` often builds whole pyramid | Single-tile / single-scale generation |
| Pass 3 batch | `thumtoo-prepare` | OK as offline tool |
| Lower-res fallback | `find_smaller_tile` | Keep; ensure coarse tiles exist early (Pass 2) |

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
| Browse / zoom now | `galapix view` + on-demand tiles (this strategy) |
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
