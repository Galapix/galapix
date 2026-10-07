<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Pixel durability and archive access policy

**Status:** normative decision (2026-09-18). Supersedes soft-ladder-as-durable
wording in older sections of [TILES.md](../TILES.md), [DESIGN.md](../DESIGN.md),
and [INTEGRATION.md](../INTEGRATION.md) where they conflict.

Related: [TILES.md](../TILES.md), [PIXEL_PIPELINE.md](PIXEL_PIPELINE.md),
biltoo `docs/THUMTOO_HOST_CONTRACT.md`.

---

## 1. Durable pixel products (only these)

| Product | Storage | API |
|---------|---------|-----|
| **Grid tiles** (256² JPEG pyramid) | Yes — `tiles` + bulk blobs | `get_tile` / `request_tile` / prepare `--tiles` |
| **LQIP** (ThumbHash / Handsum) | Yes — per blob | `get_lqip` / `request_lqip` / `ensure_lqip` |
| **Size / meta / TOC** | Yes | `get_size` / `request_size` / archive TOC |

### 1.1 LQIP generation policy (normative — do not violate)

**LQIP (ThumbHash / Handsum) must never be generated on its own.**

It is a cheap durable placeholder of **very little overall utility**. Opening or
thumbnailing the source solely to produce LQIP is **extremely expensive** relative
to its value and is **forbidden**.

**Allowed (only):**
- **Opportunistic encode** when tile pyramid build or soft/overview encode already
  holds a usable small raster in RAM (or a just-built coarsest tile JPEG).
- **Cache read** via `get_lqip` / size-reply LQIP field.
- `ensure_lqip` / `request_lqip` may encode **only from free data already in Store
  or process cache** (e.g. TileSynth / soft ≤64). They must **not** open the source.

**Forbidden:**
- Standalone `EnsureLqip` jobs that full-decode or `vips_thumbnail` the source.
- `thumtoo-prepare --lqip` as a generation phase (report-only is fine).
- Host paths that call `request_lqip` to “warm LQIP” independent of tiles/soft.
- Size-probe or open paths that generate LQIP as a side effect of needing dimensions.

**Hosts (biltoo, galapix):** schedule tiles when display needs multi-res; LQIP
appears when tiles (or free soft) already ran. Do not schedule LQIP generation.

---

**Whole-image soft / overview rasters are not durable.** They may be generated
for a single reply (or held briefly in process RAM) but must not be written as
a soft ladder into the Store.

### Reality check (Store schema ≥ 100)

`Client::get_pixels` already returns only:

1. **TileSynth** (`get_pixels_from_tiles`) when a complete scale exists and
   `allow_tile_synth` is true, or
2. **nullopt** (miss).

`handle_ensure_pixels_store` builds a soft-sized raster in memory, may fill LQIP
opportunistically, replies once, and **does not insert soft levels**. Docs that
still say “durable soft ≤ 512” describe the pre-redesign ladder epoch.

### Target policy

1. **Tiles** own multi-resolution display for biltoo (and galapix).
2. **`request_pixels` / Soft band** remain as an **ephemeral whole-image API**
   for hosts that need one QImage (filmstrip cold path, PreferCache plateau).
   Implementation may:
   - return TileSynth when tiles exist, else
   - one-shot JPEG shrink / thumbnail encode in the worker, return, discard.
3. Prefer generating **coarse tiles** over re-encoding ephemeral soft when the
   host is tile-capable (biltoo). Soft-only hosts can keep calling
   `request_pixels` / SoftOnly.
4. **`request_raster(PreferCache|Overview)`** (2026-09-26): on TileSynth miss,
   schedules **`request_tile_pyramid`** before the one-shot EnsurePixels reply so
   durable tiles advance even when the host only asked for a whole-frame sample.
5. Optional small process LRU for ephemeral soft is allowed; it is not a
   Store product and must not be required for correctness.

### Migration

- Existing soft ladder rows from schema-4 caches are unused by Store Client;
  optional GC later.
- No schema bump required to “stop storing soft” (already the case).

---

## 2. Archive access classes

Archives are **not** one access pattern. Classify at open / TOC time and pick a
dedicated extract strategy.

### 2.1 Random-access (`ArchiveAccess::Random`)

**Examples:** typical ZIP / CBZ (stored or deflate), many non-solid 7z layouts
where libarchive can seek members cheaply.

| Property | Behaviour |
|----------|-----------|
| TOC | Central directory / indexed; cheap |
| Member extract | Independent of other members |
| Interactive policy | Extract-on-demand + process extract LRU (512 MiB) |
| Concurrent cells | Coalesce same-archive jobs; shared extract cache |
| Prepare | Parallel members OK (`--jobs`) |

### 2.2 Sequential (`ArchiveAccess::Sequential`)

**Examples:** solid RAR / CBR (prefer unarr when available), solid or poorly
indexed 7z, **tar / tar.\*** (no central directory — must scan from the start
or from a prior offset), other formats where “member N” implies decoding
1…N−1.

| Property | Behaviour |
|----------|-----------|
| TOC | May require a full sequential pass |
| Member extract | Expensive if random; order matters |
| Interactive policy | **One logical cursor per archive path** (`ArchiveCursor`); windowed extract via `plan_archive_batch_window`; never fan out N independent solid streams for different members |
| Concurrent cells | Same-archive coalesce **required**; prefer continuing forward from `next_index` |
| Prepare | **Single sequential walk**: TOC → size → stage members on disk → optional tiles while hot |
| Disk staging | After first solid extract, members live under `cache_root/extract_staging/` (or `$TMPDIR/thumtoo-extract-*` for `:memory:`). Soft/tiles/re-open must hit staging, not re-decompress the RAR |

### 2.3 Classification (heuristic, refine with measurement)

```text
extension / libarchive format
  .tar .tar.gz .tgz .tar.bz2 .tbz2 .tar.xz .txz  → Sequential
  .rar .cbr (solid)                              → Sequential (unarr path)
  .zip .cbz .epub (typical)                      → Random
  .7z .cb7                                       → Sequential if solid / no seek;
                                                   else Random when seek proven
  unknown                                        → Sequential (safe default)
```

Expose something like `archive_access_class(path) → Random | Sequential` for
Client workers and prepare. When unsure, **Sequential** is the safe default
(extra ordering; still correct for random formats).

### 2.4 Shared rules (both classes)

1. TOC once; durable in index (`container_member` / equivalent).
2. Extract cache LRU for warm member bytes.
3. Image-member filter for batch windows (skip non-images in the plan).
4. Do not full-decode every member on open — size probe + LQIP + tiles per policy.

### 2.5 What already exists

- `ArchiveCursor`, `plan_archive_batch_window`, worker same-archive coalesce
  ([PIXEL_PIPELINE.md](PIXEL_PIPELINE.md)) — this is the Sequential path spine.
- Random path is the default extract-on-demand + LRU.

**Implemented (tip 307):** `ArchiveAccess` + `archive_access_class(path)`.
`Client::member_bytes` for Sequential archives plans a TOC window (cursor),
extracts once, fills the extract LRU, and returns the requested member.
Random archives keep single-member extract-on-demand. Batch/coalesce paths
already used the cursor; Sequential single-member no longer does a lone
full-stream walk per cell without caching neighbors.

---

## 3. Interactive generation order (TTFP)

Independent of archive class:

1. Size (header / sequential probe) — durable meta.
2. LQIP — durable, low priority after size.
3. Visible **tile cells** at coarsest adequate scale (JPEG DCT shrink when possible).
4. Ephemeral whole-image soft only if the host still requests Soft and tiles are
   not yet available.
5. Finer tiles on zoom; scale 0 / full only at extreme close-up or edit.

Prepare profiles:

- Default: size + LQIP (+ optional soft ephemeral is irrelevant offline).
- `--tiles --min-scale N`: coarse (or full) pyramid.
- Sequential archives: one pass that does size + LQIP + coarse tiles together.

---

## 4. biltoo product expectation

- **Tiles for display when possible** (Image, Gallery, deep zoom).
- Soft / PreferCache: TileSynth or one-shot ephemeral; no “grow durable soft ladder”.
- PathRaster Soft band may remain as a *request band name* but must not imply
  Store soft levels.

See biltoo host contract updates in the paired tip.

### Prepare ordering (tip 308)

`thumtoo-prepare` sorts sized URIs so **Sequential** archive members run in TOC
order before soft/tiles phases. Combined with `member_bytes` windowed extract
and worker same-archive coalesce, one forward pass warms neighbors instead of
N independent solid-stream walks. `--ladder` remains available but is ephemeral
(not Store-durable); prefer `--tiles`.
