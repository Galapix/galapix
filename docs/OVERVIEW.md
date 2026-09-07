<!--
SPDX-FileCopyrightText: 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Fast overview layer (design)

Stand-in pixels for “something on screen” that are **not** durable 256² grid
tiles. Complements [TILE_LOADING.md](TILE_LOADING.md).

## Why not only max_scale tiles

Develop already queues the **max_scale grid cell** as a stand-in on tile miss
(`ImageTileCache`). That is still a **real tile** (thumtoo identity, q≈80 JPEG
cell). It can be slow when thumtoo must open/decode/cut, especially for archive
members.

Old Galapix used **libjpeg DCT scale** (1/2, 1/4, 1/8) when generating tiles
(`TileGenerator::load_surface`). That path is fast for plain JPEGs and must not
write into the same cache keys as finished pyramid cells.

## Identity

| Layer | Key / store | Quality | Durable? |
|-------|-------------|---------|----------|
| Grid tiles | thumtoo `(content_id, scale, x, y)` or SQLite tiles | High | Yes |
| Overview | **Separate** — e.g. in-memory on `Image`, optional disk key `overview:<content_id>` or thumtoo preview namespace | Low | Optional cache |

Never store overview bytes as `tiles(scale,x,y)`.

## Target behaviour

1. Image becomes visible / first paint needed.
2. If no grid tile is ready, draw **overview** stretched to the image rect (if
   present or in flight).
3. When any real tile covering a region arrives, prefer the tile for that
   region; keep overview only where tiles are still missing (or drop overview
   entirely once max_scale tile exists).
4. GUI never blocks: load overview on a worker (JobManager or thumtoo executor).

## Load sources (priority order for plain files)

1. **Embedded EXIF thumbnail** (if present and large enough) — cheapest.
2. **libjpeg scale** — `surf::jpeg::load_from_file(path, jpeg_scale, &size)` with
   `jpeg_scale ∈ {2,4,8}` so the long edge is roughly ≤512–1024 px (reuse
   `TileGenerator` logic).
3. **Full decode + shrink** — last resort (non-JPEG via surfcpp), only when
   cheap paths fail.

For **archive members**, prefer one sequential member read (size + overview)
when benchmarks show split seeks lose; implementation may live in thumtoo later.

## Galapix API sketch

```text
class ImageOverview {
  enum class State { None, Requested, Ready, Failed };
  State state;
  wstdisplay::SurfacePtr surface;  // soft whole-image texture
  JobHandle job;
};

// Image holds optional ImageOverview m_overview;
// ImageRenderer::draw: if no tile coverage, draw m_overview.surface in image rect
// request_overview() on first visible frame if State::None
```

Do **not** push overview surfaces into `ImageTileCache` under a fake TileCacheId.

## Status

| Step | Status |
|------|--------|
| Design (this doc) | **Done** |
| max_scale grid stand-in | Done (tile cache) |
| `ImageOverview` + worker load for local JPEG | Not done |
| EXIF thumb path | Not done |
| Disk cache for overview | Not done (optional) |
| Archive coalesced size+overview | thumtoo / benchmark first |

## Related

* [TILE_LOADING.md](TILE_LOADING.md)
* `src/jobs/tile_generator.cpp` — existing `jpeg_scale` load
* `src/galapix/image_tile_cache.cpp` — grid stand-in requests
