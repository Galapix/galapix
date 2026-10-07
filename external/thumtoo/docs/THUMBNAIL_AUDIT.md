<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Thumbnail / ladder / tile generation audit

Status: **complete for handoff** (2026-09-09). Code fixes landed in thumtoo-083; vips/RAR numbers still pending. Companion to TODO.md
“Thumbnail generation audit + microbenchmarks”.

Audience: agents continuing the work; goal is zero open questions about
what is slow, what is fast, what is measured, and what is still missing.

## 0. Executive summary

**Time to first pixel (cold)** is dominated by: (1) archive extract when
applicable, (2) size probe which still runs LQIP/Handsum for plain images on
the same worker, (3) first tile generation which does a **full-resolution
JPEG decode** despite existing `vips_jpegload(shrink=N)` code, because Client
always passes a non-empty `decode_cache_key` or uses the file path that never
shrinks.

**Time to first pixel (warm)** should be: SQLite meta + blob get of ≤256²
JPEG + decode + RGBA + GL upload — low tens of ms if workers/GL keep up;
Galapix limits new requests to 128/frame and uploads to 64/image/frame.

**Fast:** header size probe (JPEG SOF via sequential Vips), EXIF embedded
thumb for ladder, cached get_*, ZIP non-solid random member.

**Slow:** full decode, pyramid prepare, PDF/DjVu page raster, solid RAR
walk, LQIP-on-probe (throughput), DjVu under process-wide mutex.

**Highest-impact fixes (spec §8):** implemented in thumtoo-083 (JPEG shrink
Option A, LQIP decoupled from size probe, extract-cache LRU, `tiles.source`).

## 1. Scope

| Repo | Role in this audit |
|------|--------------------|
| **thumtoo** | All durable generation: size probe, LQIP, JXL ladder, 256² JPEG tiles, archive extract, PDF/DjVu raster |
| **galapix** | Consumer: time-to-first-pixel (overview LQIP + coarse tiles), size probe session, TileProvider |

Not in scope for generation code: OpenGL upload, layout algorithms (except
where they force generation order).

## 2. Pipeline overview

```
Source URI
  → parse_location (file / archive member / page)
  → content identity (sha256 + path fingerprint)
  → size (width×height)          [fast path desired]
  → LQIP / Handsum / ThumbHash   [tiny raster]
  → ladder level (JXL, long-edge) [preview]
  → grid tiles (JPEG 256²)       [zoom]
```

Cache layout: `$XDG_CACHE_HOME/thumtoo/{index,blobs}.sqlite`.

## 3. Fast vs slow operations (taxonomy)

### 3.1 Fast (should not require full-resolution decode)

| Op | Mechanism | Code | Notes |
|----|-----------|------|-------|
| Dimensions | Header / sequential open | `probe_image_file`, `probe_image_buffer` | VIPS_ACCESS_SEQUENTIAL; for JPEG libvips reads SOF only when no pixel access |
| EXIF IFD1 JPEG thumb | Parse APP1, load small JPEG | `extract_exif_jpeg_thumbnail*` in `image.cpp` | Used by `build_ladder*` when emb_edge ≥ target |
| JPEG shrink load | libjpeg IDCT at 1/2, 1/4, 1/8 | `vips_jpegload(..., "shrink", js)` | **Only** in `build_tile_cell_buffer` when `scale > 0` and magic looks JPEG and (scale < 0 \|\| decode_cache_key empty) |
| Cached size / tile / level | SQLite + blob get | `Client::get_size`, `get_tile`, `get_pixels` | GUI-safe; no source I/O |
| Archive TOC (cached) | index rows | archive listing APIs | |

### 3.2 Medium

| Op | Mechanism | Code |
|----|-----------|------|
| `vips_thumbnail` | Shrink-on-load where loader supports it | ladder fallback after EXIF miss |
| Single-cell tile encode | Crop + JPEG q=80 | `cut_cell_from_vips` |
| LQIP encode | ThumbHash on ≤100² RGBA | `lqip.cpp` |
| Archive member extract (warm extract_cache) | RAM hit | `archive.cpp` + Client extract cache 512 MiB |

### 3.3 Slow (full resolution or equivalent)

| Op | Mechanism | Code |
|----|-----------|------|
| Full `vips_image_new_from_file` without shrink | Decode all samples | `build_tile_cell` **file path**; pyramid; ladder when no EXIF and format lacks shrink-on-load |
| `build_tile_pyramid*` | Load full → cut all scales | Always full load today |
| PDF page raster | MuPDF at chosen DPI | `pdf_mupdf.cpp` |
| DjVu page raster | ddjvu_page_render | `djvu.cpp` |
| Cold archive member extract | libarchive stream to member | Sequential walk cost for RAR especially |
| SHA-256 of whole file | CPU + I/O | size probe path (now path+mtime cached, thumtoo-070) |

## 4. File-by-file (thumtoo)

### 4.1 `src/image.cpp` — core pixel generation

**Probe (`probe_image_file` / `probe_image_buffer`)**
- Refuses PDF/DjVu (thumtoo-073) — correct.
- Uses `VIPS_ACCESS_SEQUENTIAL`. For JPEG this is **header-class** if no
  pixel operation follows; width/height come from SOF.
- Does **not** use pure libjpeg `jpeg_read_header` path; relies on Vips.
- Format string from extension / hint only.

**Ladder (`build_ladder` / `_buffer` / `_rgb`)**
- Header open for long_edge → `pick_preview_edge`.
- JPEG: try EXIF embedded thumb first; if large enough, JXL-encode that
  (or thumbnail_image down). Stats: `exif_thumb_hits`.
- Else `vips_thumbnail` (shrink-on-load for JPEG/PNG where supported).
- **Does not** emit multi-edge ladder in one call when max_edge_limit is
  set for prepare; prepare policy is one edge (see prepare CLI).
- PDF/DjVu refused.

**Tiles**
- `jpeg_shrink_factor_for_scale`: scale≥3 → 8, ≥2 → 4, ≥1 → 2.
- `build_tile_cell_buffer`: JPEG shrink **only** when
  `scale < 0 || decode_cache_key.empty()`. If a decode cache key is set
  (ladder_acquire_level path), loader is full `vips_image_new_from_buffer`
  with **no shrink**.
- `build_tile_cell` (filesystem path): **never** calls `vips_jpegload` with
  shrink. Always full file open via ladder_acquire_level or direct
  `vips_image_new_from_file`.
- `build_tile_pyramid*`: always full load then `cut_pyramid_from_vips`.
- Interactive path in Client prefers single-cell (`request_tile`); prepare
  uses pyramid.

**Gaps / bugs in image.cpp**
1. **Asymmetry file vs buffer**: file path tiles miss JPEG shrink fast-path.
2. **ladder_acquire_level defeats JPEG DCT shrink**: the in-process shrink
   ladder (`g_ladders`, max 4 entries) always `load_full()` once then
   `vips_shrink` ×2 per scale step. Good when many tiles of one image are
   requested; **bad for cold single coarse cell** (still full decode).
3. **Client always passes non-empty decode_cache_key** for archive (`a:…`)
   and HTTP (`h:…`) interactive cells → the `vips_jpegload` shrink=2/4/8
   branch in `build_tile_cell_buffer` is **dead code on the hot path**.
   Plain files use `build_tile_cell` which never shrinks either.
4. **No quality / fast-path metadata** on stored tiles (all look the same).
5. **Pyramid ignores shrink**: generating scale 3 still decodes scale 0
   pixels first.
6. Embedded EXIF thumb used for ladder, **not** for tile cells or LQIP
   source when only a tiny preview is needed.
7. `path_looks_jpeg` is extension-based; buffer path uses magic bytes — good
   for archives renamed members.

### 4.2 Client call graph (interactive tile) — verified

```
handle_ensure_tiles (non-pyramid):
  archive → member_bytes → build_tile_cell_buffer(..., dkey="a:…")  // NO jpeg shrink
  http    → fetch_http_cached → build_tile_cell_buffer(..., dkey="h:…")  // NO jpeg shrink
  file    → build_tile_cell(path)  // ladder_acquire_level full load
  pdf/djvu → dedicated raster (rgb888 live; durable JPEG only above threshold)
```

Reply-before-durable-store is already implemented (live paint first).

### 4.3 `src/client.cpp` — orchestration

**Cache-only reads (GUI-safe)**
- `get_size` / `get_meta` / `get_lqip` / `get_tile` / `get_pixels`: SQLite +
  blob only. Warm path has no source I/O.

**Size probe (`handle_probe_size`)**
- If width/height already Ready/Incomplete: return immediately; optional
  LQIP *backfill* if missing (`store_lqip_if_missing` on file path).
- PDF/DjVu: layout size only (72dpi media box / ddjvu); status Incomplete;
  **no** LQIP on probe (thumtoo-070).
- Archive member: `member_bytes` (extract cache) → `probe_image_buffer` →
  status Incomplete. Probe itself is sequential Vips open (header-class for
  JPEG). **Does not** call LQIP in the archive branch of the final
  size_out block (only plain file://).
- HTTP: fetch → probe buffer → **does** `store_lqip_if_missing` on bytes.
- Plain file://: probe → Incomplete → **`store_lqip_if_missing` on path**
  (LQIP still coupled to first size probe for local images).

**LQIP (`store_lqip_if_missing` / `ensure_lqip`)**
- Prefer Handsum (magic `FE D6`); replace legacy ThumbHash.
- Raster sources:
  - RGB888 → encode
  - path → `lqip_thumbhash_from_file` → `vips_thumbnail(..., 32)`
  - buffer → `vips_thumbnail_buffer(..., 32)`
- PDF/DjVu in `ensure_lqip`: dedicated page raster at edge 64 (not Vips).
- Archive in `ensure_lqip`: full member extract then thumbnail_buffer 32.
- **Policy (thumtoo-086):** LQIP is **not** generated on size probe.
  Workers store LQIP only after the first durable tile (or EnsurePixels RGB
  ladder). That is the whole mechanism: on a cold open LQIP is simply not in
  the cache yet while better tiles paint; successive opens find it stored.
  No Galapix “warm/cold” display special-case is required or desired.

**thumtoo-088:** Handsum after the first tile must not run on the tile
worker inline — it starved further cells (black gallery). `request_lqip`
queues `EnsureLqip` behind Probe/Tiles work. Cold-open underlay is the
max-scale tile itself, not LQIP.

**Tiles**
- Warm: `get_tile` then reply; no decode of source.
- Cold interactive: see §4.2 (no JPEG shrink on hot path).
- Reply-before-durable-JPEG-store for live paint (good for TTFB).
- Extract cache 512 MiB; on overflow **clears entire map** (not LRU).

**Workers**
- Shared SQLite WAL; recursive_mutex on DB/BlobStore (thumtoo-051).
- Interactive tile queue FIFO (starve fixes in galapix history).

### 4.4 `src/archive.cpp` + client archive batching

- `read_archive_toc`: sequential libarchive headers only (skip data).
- `extract_archive_members`: one sequential pass; collect wanted members into
  a map (good for cold multi-member batches).
- Client worker **coalesces** same-archive same-`JobKind` jobs from the queue
  into one extract pass; feeds `preextracted` into handlers.
- **Warm skip:** for EnsureTiles, if `get_tile` already hits, **no extract**
  (avoids full RAR cost on every open of a cached gallery).
- `member_bytes`: preextracted → extract_cache → single-member extract.
- Extract cache: 512 MiB; overflow clears **entire** map (not LRU) — §8.4.
- ZIP non-solid: random member is cheap (see MICROBENCH_RESULTS). Solid RAR:
  sequential dependency — coalesce batch still one pass, but cannot seek.

### 4.5 LQIP encode (`lqip.cpp` / `handsum.cpp` / `image.cpp` helpers)

- Encode cost is small vs obtaining RGBA.
- `vips_thumbnail` edge 32 is **medium** (JPEG shrink-on-load), not full-res
  for JPEG — but still non-trivial I/O + decode, and runs on size probe.
- Does **not** try EXIF embedded thumb first (ladder does; LQIP does not).

### 4.6 `src/pdf.cpp`

| Op | Cost class | Notes |
|----|------------|-------|
| `pdf_page_size_72dpi` / layout | Fast | Media box; TLS layout cache per path+page |
| `pdf_rasterize_page(max_edge)` | Slow | Full page at DPI scaled to long edge |
| `pdf_rasterize_page_region` | Medium–slow | MuPDF crop at target DPI; O(tile) pixels |
| `pdf_render_tile_cell` | Medium | Prefer region; **fallback full-page** if region size mismatch >2px |
| Document open | Medium | TLS per-worker document cache (path+mtime) |

- Layout DPI constant (`kPdfLayoutDpi`, historically 144) defines scale 0.
- Live interactive: rgb888; durable JPEG only for scale ≥ `kPdfMinDurableTileScale`.
- Galapix allows live min_scale down to **−8** (~36k dpi) — each cell still
  region-renders, so memory is O(256²), but CPU grows with DPI.
- Fallback full-page path is a footgun for large pages when region API
  misbehaves (rare size mismatch).

### 4.7 `src/djvu.cpp`

| Op | Cost class | Notes |
|----|------------|-------|
| page layout size | Fast–medium | Needs decoded page dimensions |
| `djvu_rasterize_page` | Slow | Full page; shared process-wide doc cache + mutex |
| region render | Medium–slow | Crop rect; y_direction top-down (thumtoo-074) |
| blank page | Fast | solid white on render failure (thumtoo-075) |

- **Process-wide mutex** on all DjVu API (thumtoo-071): correct for ddjvu,
  serializes workers — high `--jobs` does not parallelize DjVu well.
- `ddjvu_cache_clear` after each page render — avoids unbounded RAM, may
  re-decode shared structure across pages.

### 4.8 Schema / blobs

**index.sqlite (`schema.sql` v2)**
- `content`: size, status, **lqip BLOB + lqip_kind**
- `locators`, `archive_entries`, `levels` (metadata only), `tiles` (metadata)
- `tiles` PK `(content_id, scale, x, y)` — columns width/height/codec/quality
- **No** `source` / fast-path flag on tiles

**blobs.sqlite**
- `level_blobs`, `tile_blobs`, `http_bodies` — WAL, busy_timeout 5000
- `recursive_mutex` on BlobStore (and Database) for multi-worker
- get/put prepare statements each call (no statement cache) — optional later
- No blob size quota / eviction beyond manual `thumtoo-gc`

Warm `get_tile`: index row + blob SELECT of one JPEG ≤256² — should be
milliseconds; measure under load when SQLite contended.

## 5. Galapix consumption

### 5.1 Time-to-first-pixel paths

| Stage | Cold cache | Warm cache |
|-------|------------|------------|
| Size | `request_size` + worker probe (+ LQIP for plain files) | `get_size` ~instant |
| Soft underlay | `get_lqip` miss until probe fills; UI never `ensure_lqip` | `get_lqip` + Handsum decode on main |
| Coarse tile | extract + **full** decode + RAM shrink ladder + JPEG | `get_tile` + JPEG decode + GL |
| Overview levels | `request_pixels` / max_scale tile after budget | cache hit |

### 5.2 `ThumtooTileProvider` (`src/thumtoo/thumtoo_tile_provider.cpp`)

- `create_from_size`: pure local; max_scale matches thumtoo loop (fit in 256²).
- `create`: **blocking** `request_size` + `drain` — must not be used in a
  per-URI loop on open (ViewerCommand uses SizeProbeSession batch instead).
- `request_tile` / `request_tiles`: callback on Client worker; JPEG/rgb888
  → RGBA8 **on worker** (good — not GUI).
- `surface_from_tile_blob`:
  - rgb888: per-pixel expand to RGBA8 (PDF/DjVu live).
  - JPEG: `surf::jpeg::load_from_mem` then convert to RGBA8 if needed.
- Warm TTFB still pays: SQLite get + JPEG decode of ≤256² + RGBA convert +
  queue to main + GL upload. Should be low tens of ms for many tiles if
  workers keep up; stampede limited by galapix job budget.

### 5.3 `ImageOverview`

- thumtoo: **only** `get_lqip` on GUI; retry each frame (galapix-082).
- Non-thumtoo: `OverviewLoadJob` + libjpeg DCT scale.
- `ensure_levels` after grid budget; ≤128 long-edge stays LQIP-only.

### 5.4 Existing measurement tools

- `thumtoo-bench`, `open_phase_bench`, `GALAPIX_OPEN_TIMING`.
- New: `thumtoo-microbench-decode`, `docs/MICROBENCH_RESULTS.md`.

### 5.5 `SizeProbeSession` (galapix)

- Open path queues `request_size` for all URIs, constructs placeholder Images,
  launches viewer, then `start_drain` on a **background thread**.
- Main-thread `tick()` polls `get_size` and attaches `create_from_size` when
  ready — does **not** wait on size callbacks.
- **Interaction with LQIP:** `handle_probe_size` upserts size, then
  synchronously runs `store_lqip_if_missing` for plain files, then posts
  `size_cb`. Size is in SQLite before LQIP finishes, so `tick()` can attach
  providers while the same worker is still encoding Handsum — good for
  layout. Throughput of the worker pool still pays LQIP cost per plain
  file before that worker takes the next probe (multi-worker mitigates).

### 5.6 `ImageTileCache` request path

- Global per-frame budget: `begin_frame_request_budget(128)` in `Viewer`
  (raised from historical 48).
- `issue_requests`: sort **finer scale first**, then distance to focus.
  Coalesces into `request_tiles` batch for ThumtooTileProvider.
- Failed/aborted cells retried up to 3 attempts (important for live PDF).
- `find_smaller_tile`: **lookup only** (no request from draw path) — avoids
  O(max_scale) job storm.
- GL upload cap: 64 tiles per image per frame.
- `cleanup()` aborts in-flight only; retains all SUCCEEDED surfaces (memory
  grows with browsing).
- Interactive thumtoo queue: **FIFO** (`enqueue(..., front=false)`); same-cell
  supersede drops obsolete pending work. (Older LIFO starved archive grids.)

### 5.7 End-to-end cold open (many JPEGs in ZIP)

```
expand TOC → request_size×N → drain workers:
  per member: extract (sequential archive coalesce) → probe header
              → upsert size → LQIP thumb32+Handsum  [plain-like member]
UI: get_size → provider → overview get_lqip (may still miss)
    → issue_requests coarse cells → full JPEG decode path (§4.2)
    → reply RGB/JPEG → worker RGBA → upload ≤64/frame
```

Warm open skips extract/probe/LQIP/decode-source; still pays get_tile +
JPEG decode 256² + RGBA + GL.

## 6. Things we could do but do not (yet)

1. Systematic EXIF / embedded thumbnail for **any** first pixel (tiles + LQIP).
2. JPEG shrink on **file-path** `build_tile_cell`.
3. Shrink-aware pyramid (decode at max needed shrink, build finer from
   that only when requested — or decode once at shrink=1 for prepare HQ).
4. DB flag `tile_quality` / `decode_path` (full | jpeg_shrink_N | embedded).
5. Progressive JPEG / hierarchical scans for intermediate display (rare).
6. libjpeg-turbo direct API for header-only and scaled decode microbench
   baseline vs Vips.
7. Archive: memory-map ZIP central directory; compare libarchive vs CLI
   tools for random member extract latency.
8. Store original SOF dimensions without opening pixels for more formats
   (PNG IHDR, etc. — Vips sequential usually already cheap).
9. Match historical Galapix `TileGenerator` DCT scale behaviour for
   interactive thumtoo JPEG cells (§6d / §8.1).
10. Automatic cache size budget (gc is manual only today).
11. Magic-byte format sniff for extensionless / wrong-extension members.
12. Statement cache on Database/BlobStore hot gets.

## 6b. Offline prepare & instrumentation

### `thumtoo-prepare`

Phases: `prepare_paths` (expand archive/PDF/DjVu ≤512 pages → `request_size`)
→ optional `--ladder EDGE` (`request_pixels`, single JXL edge not multi-rung)
→ optional `--tiles` (`request_tile_pyramid`, full scales in range).

- `--jobs N`: Client worker pool (default HW concurrency, max 32).
- `--stats` / `--stats-line`: print `BuildStats` after run.
- PDF/DjVu page expansion capped at **512** pages per document in prepare.

### `BuildStats` (`build_stats.hpp`)

Accumulates **thread CPU-ish time** (overlapping workers → sum can exceed
wall): `archive_extract_ns`, `image_load_ns`, `shrink_ns`, `jpeg_encode_ns`,
`thumb_ns`, `jxl_encode_ns`, plus counters levels/tiles/probes/`exif_thumb_hits`.

`summary_pretty` reports wall since `reset()` and approx parallel factor
`cpu-sum/wall`. Use this to see whether load vs jpeg encode dominates prepare
— not a substitute for microbench_decode.

### `expand_media_uris` / `prepare_paths`

- Archives → image member URIs via TOC refresh.
- PDF/DjVu → `//page:N` URIs.
- Skips Ready locators on re-prepare when size already known.

### Interactive vs prepare tile cost

| Mode | Scales generated | JPEG shrink | Full load |
|------|------------------|-------------|-----------|
| `request_tile` | one cell one scale | dead on hot path | yes today |
| `request_tile_pyramid` / prepare `--tiles` | scale range full grid | no | yes always |

Prepare is intentionally expensive offline; interactive should not share that
cost model — hence §8.1.


## 6c. Cache maintenance & format classification

### `thumtoo-gc`

Manual only — **no automatic eviction** / size budget on blobs.

| Flag | Effect |
|------|--------|
| `--min-scale N` | DELETE tiles/tile_blobs with `scale < N` (keeps coarser) |
| `--orphans` | Content with no locators (+ blobs) |
| `--dead-paths` | Locators whose `outer_path` missing on disk; then orphans |
| `--dry-run` | Report only |

LQIP stays on content until the content row is purged. Fine-scale tile
drop is the main disk reclaim lever for zoom caches.

### `thumtoo-status`

Read-only inspect: summary / locators / content / levels / tiles / archives.

### `format.hpp` / `format.cpp`

Extension-based `PathKind`: Image / Pdf / Djvu / Archive / Unsupported.

Image exts include jpeg/png/gif/bmp/webp/jxl/tiff/heic/heif/avif. Archives
include zip/cbz/cbr/rar/7z/tar variants. MIME list for desktop integration.

**Not** magic-byte sniffing for path classification (buffer tile path does
use JPEG SOI magic). Misnamed files may take wrong open path.

## 6d. Historical Galapix path (non-thumtoo) — contrast

When `HAVE_THUMTOO` is off or `--no-thumtoo`, overview uses
`OverviewLoadJob` → `TileGenerator::load_surface`:

```cpp
// JPEG only: jpeg_scale = min(pow2(min_scale), 8)
jpeg::load_from_file(path, jpeg_scale, &size);  // libjpeg DCT scale
```

That **does** the fast coarse load thumtoo’s interactive path was meant to
mirror (`vips_jpegload` shrink). Non-JPEG falls back to full
`surface_factory().from_file`.

Implication: a gallery of JPEGs on the **old** Galapix path can show soft
overviews cheaper than thumtoo today for cold coarse tiles — until §8.1
lands. Thumtoo still wins on durable cache, archives, PDF/DjVu, and warm
hits.

## 6e. Content hashing

- Size probe upgrades provisional ids to `sha256:…` after hashing file or
  member bytes (PDF/DjVu: hash file + `:page:N`).
- `sha256_file_hex` path+mtime cache (thumtoo-070) avoids re-hash on every
  probe for multipage docs.
- Same bytes → same content_id → shared ladder/tiles across renames when
  locators are updated.

## 7. Benchmark plan (to implement)

Corpus: synthetic JPEGs at 4K, 8K, 16K; PNG; JXL; ZIP/RAR of many JPEGs;
single multipage PDF/DjVu if tools present.

| Bench | Metric |
|-------|--------|
| Header-only size | ns/file, JPEG/PNG |
| Full decode | ms, peak RSS |
| jpegload shrink 2/4/8 | ms + optional PSNR vs full+downsample |
| EXIF thumb extract + decode | ms |
| Ladder edge 256 | ms cold/warm |
| Tile cell scale 0 vs 3 | ms |
| Pyramid full | ms |
| Archive TOC | ms |
| Archive random member × N | ms libarchive vs unzip/unrar |
| Client get_tile warm | µs |
| End-to-end request_tile cold | ms |

Output: machine-readable JSON + markdown table in this doc.

## 7b. Policy constants (`include/thumtoo/constants.hpp`)

| Constant | Value | Role |
|----------|-------|------|
| `kLadderEdges` | 128…2048 | Display ladder long edges |
| `kDefaultJxlQuality` | 80 | Ladder encode |
| `kTileSize` | 256 | Grid cell |
| `kDefaultTileQuality` | (see constants) | JPEG tiles |
| `kPdfLayoutDpi` | 144 | Scale 0 / size layout |
| `kPdfMinDurableTileScale` | (see file) | Below this: live only |
| `kArchiveMaxMemberUncompressedBytes` | 512 MiB | Extract cap |
| `Client::kExtractCacheMaxBytes` | 512 MiB | Process RAM cache |
| `kLadderCacheMaxEntries` | 4 | In-process shrink ladder |
| Galapix frame tile budget | 128 | `begin_frame_request_budget` |
| Galapix GL uploads/image/frame | 64 | `kMaxUploadsPerFrame` |

## 6f. Scanned PDFs and image-heavy pages (historical: Poppler; now MuPDF)

> **Superseded (2026-10-07).** The image-heavy heuristic, the full-page RGB
> cache and the refusal of live tiles on scans are gone. Pages are classified
> by a profiling device (`PdfPageProfile`), raster pages cap at their native
> dpi, and images decode once per page and level. See TILES.md "PDF
> rendering". The text below is history.

**Document** is already TLS-cached per Client worker (`cached_pdf_document`).
**Page** was re-`create_page`d on every region render — now TLS-cached too.

**Region vs full page:** `render_page(..., px,py,pw,ph)` still walks page
content. For scanned pages (one large image XObject) Splash often **re-decodes
the full JPEG** for each crop. That was Poppler behaviour (removed); MuPDF region tiles use a display list
away handles.

**Full-page RGB cache:** only for **scale ≥ 0** and long edge ≤ 4096. Never at
deep live zoom (would be huge).

**Image-heavy detection (thumtoo-093):**
- With **MuPDF**: `fz_stext` image blocks → image count +
  coverage fraction (box area / media box). Coverage ≥ 0.45 → image_heavy.
- Without glib: sparse text (`text_chars / page_points²` < threshold) →
  image_heavy (scanned heuristic).
- **Live tiles** (scale < 0) **refused** when image_heavy; Galapix sets
  `min_scale = 0` for those pages. Vector pages keep deep live zoom.

## 7c. Sleeping USB / GUI thread policy

Source media may take **10–30 seconds** to spin up. Rules:

1. **No source filesystem I/O on the GUI thread** — not even `is_regular_file`,
   `pdf_page_count`, or archive TOC (Galapix `open_paths` does these on a worker).
2. `get_*` APIs are SQLite/blob only (local cache). Keep the thumtoo cache on
   a fast volume, not the sleeping USB that holds the library.
3. `request_*` / `ensure_lqip` / probe / extract run on Client workers only.
4. Galapix must never call `ThumtooTileProvider::create` (blocking `drain`) from
   the GUI; use `create_from_size` after `get_size` or SizeProbeSession.

## 8. Proposed fixes (not implemented — discuss before code)

### 8.1 Interactive JPEG shrink on the hot path

**Problem:** `build_tile_cell_buffer` only uses `vips_jpegload(shrink=N)` when
`decode_cache_key.empty()`. Client always passes `a:…` / `h:…` keys; file path
uses `build_tile_cell` → `ladder_acquire_level` full load.

**Option A — single-cell fast path (preferred for TTFB)**  
When interactive request is **one cell** and no other scale for that key is
cached yet:

1. If magic/path is JPEG and `scale > 0`, load with `shrink = jpeg_shrink_factor_for_scale(scale)`.
2. Crop cell; JPEG-encode; reply.
3. Optionally **do not** populate the full-res RAM ladder (or populate only
   the shrink level) so memory stays low.

When the same image later needs multiple scales/tiles, fall back to full load
+ ladder (current behaviour) or progressively fill ladder from finest
requested.

**Option B — shrink-aware ladder_acquire_level**  
Change `load_full` to `load_at_min_scale(scale)`: first open uses
`jpegload(shrink=min(8, 2^scale))`, store as level `applied_steps`, then
`vips_shrink` only remaining steps. Non-JPEG still full load.

**Option C — leave ladder; fix only file path**  
Mirror buffer JPEG branch into `build_tile_cell` when scale>0 and ladder miss.
Does not fix archive/HTTP unless decode_cache_key is empty for single-shot.

Recommend **A** for cold coarse tiles (gallery overview), **B** if multi-tile
same image is the common case after first paint.

### 8.2 Decouple LQIP from size probe

Size callback should not wait on Handsum. Enqueue LQIP as a separate low-priority
job after size is stored (or only on `ensure_lqip` / EnsurePixels). Galapix
already tolerates missing LQIP and retries `get_lqip`.

### 8.3 Optional `tile_source` / quality column

`tiles` table: add `source TINYINT` — `0=full`, `1=jpeg_shrink`, `2=embedded`,
`3=pdf_region`, … Re-generate if UI requests HQ and only fast-path exists.
Not required for correctness; helps cache policy and debugging.

### 8.4 Extract cache LRU

Replace clear-all-on-overflow with size-based LRU so large albums do not
thrash the entire 512 MiB map after one oversized member.

## 9. Progress log


- 2026-09-09: Initial taxonomy and image.cpp findings written.
- 2026-09-09: Client/LQIP/archive/galapix paths documented; JPEG shrink
  dead on interactive hot path confirmed.
- 2026-09-09: Pillow microbench corpus + results in
  `docs/MICROBENCH_RESULTS.md` (size_only ~0.03ms; draft scale only
  1.3–2.3× vs full on large JPEG; PIL thumb32 on 8K ~207ms).
- 2026-09-09: `tools/microbench_decode.cpp` + CMake target
  `thumtoo-microbench-decode` (needs vips build).
- 2026-09-09: PDF/DjVu cost tables; Galapix `ThumtooTileProvider` decode
  path; fix proposals §8 (JPEG shrink, LQIP decoupling, tile_source, LRU).
- 2026-09-09: SizeProbeSession, ImageTileCache budget (128), FIFO tiles,
  end-to-end cold ZIP open diagram (§5.5–5.7).
- 2026-09-09: Archive coalesce + warm extract skip; schema/blob_store;
  ZIP stored/deflate microbench numbers.
- 2026-09-09: Executive summary; prepare/BuildStats/expand; interactive vs
  prepare cost table.
- 2026-09-09: gc/status/format; non-thumtoo TileGenerator DCT contrast;
  hashing notes; audit marked complete for handoff.
- Code next (when approved): Option A JPEG shrink; LQIP decoupling;
  extract LRU. Numbers next: vips microbench under nix; RAR samples.

## 10. Coverage checklist

| Topic | Documented |
|-------|------------|
| Fast vs slow taxonomy | §3 |
| image.cpp probe/ladder/tiles/JPEG shrink dead path | §4.1–4.2 |
| client size/LQIP/tiles/queue/coalesce | §4.3–4.4 |
| archive extract + warm skip | §4.4 |
| LQIP/Handsum obtain | §4.5 |
| PDF/DjVu raster | §4.6–4.7 |
| Schema/blobs | §4.8 |
| Galapix provider/overview/SizeProbe/ImageTileCache | §5 |
| Prepare/BuildStats/expand | §6b |
| gc/status/format/hashing | §6c–6e |
| Non-thumtoo DCT overview | §6d |
| Benchmark plan + Pillow/ZIP numbers | §7, MICROBENCH_RESULTS |
| Fix proposals | §8 |
| Policy constants | §7b |
| Vips in-tree numbers | pending toolchain |
| RAR solid numbers | pending samples |
| Code fixes | not started |

