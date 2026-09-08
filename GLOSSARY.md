<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Galapix glossary

Domain terms used in Galapix, its tile pipeline, and the optional
[thumtoo](https://github.com/Grumbel/thumtoo) backend. Related design notes:
[docs/TILE_LOADING.md](docs/TILE_LOADING.md), [docs/THUMTOO.md](docs/THUMTOO.md),
[docs/OVERVIEW.md](docs/OVERVIEW.md).

---

## Viewing and layout

| Term | Meaning |
|------|---------|
| **Workspace** | Collection of images (and other items) laid out in 2D world space. Owns visibility, prepare, and draw passes. |
| **WorkspaceItem** | One item in the workspace (typically an **Image**). |
| **Image** | A single picture in the workspace: URL, layout transform, **TileProvider**, **ImageTileCache**, **ImageRenderer**, **ImageOverview**. |
| **Viewer** | Top-level view: camera (pan/zoom), tools, draw loop, redraw scheduling. |
| **cliprect** | Axis-aligned rectangle in world space currently visible on screen. |
| **zoom / scale (camera)** | Viewer magnification applied to world coordinates (distinct from **tile scale**). |
| **Layout / layouter** | Algorithm that positions images in the workspace (regular grid, tight packing, etc.). |

---

## Tile pyramid geometry

| Term | Meaning |
|------|---------|
| **Tile** | One fixed-size cell of image data, almost always **256×256** pixels at its pyramid level. Identified by `(scale, x, y)`. |
| **Tile size / kTileSize** | Edge length of a grid cell in pixels; Galapix/thumtoo convention is **256**. |
| **Scale (tile scale, tilescale)** | Integer pyramid level. **0 = full resolution.** Higher values are **coarser** (each step halves linear resolution: scale *s* ≈ source / 2^s). |
| **min_scale / max_scale** | Finest and coarsest scale the provider will serve. For thumtoo, **max_scale** is the level where the whole image fits in a single 256² tile (not the historical “≤8px edge” formula). |
| **Negative scale** | Denser than scale 0 (e.g. PDF live tiles at higher DPI). Not all providers support this. |
| **Pyramid** | Full set of scales for an image: scale 0 plus successive halves up to max_scale. |
| **Grid / tile grid** | Integer lattice of tiles covering the image at a given scale. |
| **Edge tile** | Tile on the right or bottom border; may be smaller than 256×256 in content (often padded for filtering). |
| **Ancestor / parent tile** | Coarser cell covering the same region: roughly `(scale+1, x/2, y/2)`. |
| **Child tiles** | Four finer cells under one parent at `scale-1`. |

---

## Loading, cache, and draw pipeline

| Term | Meaning |
|------|---------|
| **TileProvider** | Abstract source of tiles (`request_tile`, size, min/max scale). Implementations: **ThumtooTileProvider**, legacy generators, Mandelbrot, etc. |
| **ImageTileCache** | Per-image in-memory map of tile cells: request state, software/GL surfaces, upload queue. |
| **mark_tile_needed** | Record that a cell (and often its ancestor chain) should be loaded this frame. Does not start I/O by itself. |
| **issue_requests** | Turn the marked set into real provider jobs, subject to budget and cache hits. |
| **prepare / prepare_tiles** | Per-frame **update** phase: overview ensure, cancel obsolete jobs, mark needed tiles, process uploads. No pure drawing. |
| **draw (pure)** | Lookup cached surfaces and paint; should not start heavy work. |
| **request budget** | Global per-frame limit on **new** provider jobs (`try_consume_request_budget`). Prevents stampeding thousands of jobs on first paint. |
| **Cache-only mode** | Debug mode (`U` by default): no new provider jobs; only in-memory cache is used. In-flight jobs may still finish. |
| **lookup_tile** | Read cache entry for `(x,y,scale)` without requesting. Missing cells are treated as “still loading” for placeholders. |
| **Stand-in** | Coarser (or otherwise already-ready) tile drawn upscaled when the exact cell is not ready (`find_smaller_tile`). |
| **Purple (placeholder)** | Magenta/purple fill for a cell that is loading and has **no** stand-in surface. Indicates “nothing ready for this cell yet.” |
| **Pending uploads** | Decoded software surfaces waiting in `m_tile_queue` for **GL texture upload** on the main thread (`process_queue`). |
| **Pending requests** | Cache entries still in `SURFACE_REQUESTED` (job in flight or not yet uploaded). |
| **cancel_jobs** | Abort obsolete in-flight work when the visible rect/scale changes; coarser stand-in jobs are often kept. |
| **stable_request_scale** | Hysteresis on the requested pyramid scale so rapid wheel zoom does not thrash adjacent levels every frame. |
| **Tile debug (`V`)** | Overlay: translucent tint by **displayed** pyramid scale, outline; helps see exact vs stand-in vs loading. |

---

## Soft first layer (overview / levels)

| Term | Meaning |
|------|---------|
| **Overview (ImageOverview)** | Soft whole-image preview drawn under the tile grid. Intended as the **first** thing on screen while tiles refine. |
| **Levels (thumtoo)** | Durable **preview ladder** in thumtoo: single images at long-edge sizes in `{128, 256, 512, 1024, 2048}`, typically **JPEG-XL**, keyed by content—not the 256² grid. API: `get_pixels` / `request_pixels(max_edge)`. |
| **Ladder** | Same family as levels: policy edges for previews. Distinct from the **tile pyramid**. |
| **max_edge** | Long-edge pixel limit when requesting a level (e.g. 512 for overview). |
| **skip_grid** | Draw path that shows only overview (gallery / tiny on-screen size) when overview is Ready. |

**Ordering (intended):** levels/overview → coarser stand-in tiles → exact-scale tiles.

---

## thumtoo backend

| Term | Meaning |
|------|---------|
| **thumtoo** | External library/cache: SQLite index + blob store for sizes, levels, grid tiles, archives, PDF pages. |
| **Client** | thumtoo API entry: `request_size`, `request_pixels`, `request_tile`, prepare tools, etc. |
| **URI (thumtoo)** | Location string: `file:///…`, `//archive:member`, `//page:N` (PDF), `http(s)://…`. |
| **content_id** | Stable content identity (prefer `sha256:…`; provisional ids until hashed). |
| **Locator** | Maps a URI to path/member metadata and content_id. |
| **TileBlob** | Encoded grid cell payload: scale, x, y, size, **codec**, bytes. |
| **codec** | Payload encoding: `jpeg` (durable tiles), `rgb888` (live interactive/PDF cells), `jxl` (levels). |
| **Interactive tile path** | Single-cell ensure for the viewer: ideally one crop delivered quickly (rgb888 live; JPEG stored durably). |
| **Pyramid / prepare path** | Batch build of many scales or whole grids (`thumtoo-prepare`, pyramid jobs)—not the interactive hot path. |
| **Decode ladder cache** | In-process thumtoo cache of shrink levels so many cells share one source decode. |
| **Executor** | Delivers completion callbacks (Galapix posts them to the GUI thread via **ThumtooCallbackQueue**). |
| **ThumtooTileProvider** | Galapix `TileProvider` wrapping a shared `thumtoo::Client` and one URI. |

---

## Surfaces and rendering

| Term | Meaning |
|------|---------|
| **Software surface** | CPU-side pixel buffer (`surf::SoftwareSurface`), often RGBA8 after decode. |
| **GL surface / wstdisplay Surface** | GPU texture wrapper used for drawing. |
| **surface_from_software** | Upload software pixels to a GL texture (main thread; may pad edge tiles). |
| **rgb888 live cell** | Uncompressed RGB tile bytes from thumtoo; Galapix skips JPEG decode. |
| **Job / JobHandle / JobManager** | Async unit of work and its cancel/finish handle; classic Galapix worker path (overview load, legacy generators). |

---

## Debug and UI

| Term | Meaning |
|------|---------|
| **Tile requests toggle (`U`)** | Enable/disable starting **new** tile/level provider jobs (cache-only when off). |
| **Tile debug toggle (`V`)** | Scale-tint overlay on cells. |
| **Toast / notification HUD** | Short on-screen messages for those toggles (and similar). |
| **GALAPIX_OPEN_TIMING** | Env flag for first-paint / fill timing logs. |

---

## Quick “do not confuse”

| This | Is not the same as |
|------|---------------------|
| **Tile scale** (pyramid level) | Camera **zoom** |
| **Levels / ladder** (whole-image JXL preview) | **Grid tiles** (256² pyramid cells) |
| **Overview** (soft underlay) | Exact-scale tile coverage |
| **Stand-in** (coarser tile) | **Purple** (no surface at all) |
| **Pending uploads** (GL queue) | **Pending requests** (provider jobs) |
| **Interactive request_tile** | Full **pyramid prepare** |
| **content_id** | File path / URI (one content may have many locators) |

---


## Inline LQIP (tiny gallery stamps)

| Term | Meaning |
|------|---------|
| **LQIP** | Low-Quality Image Placeholder: a few dozen bytes that approximate the image for instant gallery cells. |
| **ThumbHash** | DCT-based LQIP (~25–37 bytes). Stored on the thumtoo **content** row (`content.lqip`), not in blob storage. |
| **lqip_kind** | Discriminator for the blob format (`1` = ThumbHash). |
| **Handsum** | Alternate fixed-size LQIP (48–147 B); not wired yet — candidate if we need sharper fixed columns. |

**Stack:** ThumbHash (row) → levels (blob) → grid tiles.

## See also

- [docs/TILE_LOADING.md](docs/TILE_LOADING.md) — priority order and backend behaviour  
- [docs/THUMTOO.md](docs/THUMTOO.md) — integration and CLI  
- [docs/OVERVIEW.md](docs/OVERVIEW.md) — project overview  
- thumtoo [TILES.md](https://github.com/Grumbel/thumtoo/blob/master/TILES.md) / [DESIGN.md](https://github.com/Grumbel/thumtoo/blob/master/DESIGN.md) — normative backend constants  
