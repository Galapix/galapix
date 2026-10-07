<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# ECS rewrite of the image / tile pipeline

Status (2026-10-07): **stage 1 done** — tiles run on `thumtoo::lod`
(thumtoo `7e87428`); `ImageTileCache`, `ImageRenderer`, `ImageOverview`
are gone. Stages 2–3 (EnTT workspace, tools) are next.

## Why

Tiles regularly get stuck at the wrong resolution. The tile path has grown
by patching symptoms (TODO.md has a long trail: reclaim of lost callbacks,
stand-in requests that never drained, dead-cell retry caps, budget tweaks).
State for one image is spread over `Image`, `ImageTileCache`,
`ImageRenderer`, `ImageOverview`, `JobHandle`s and thumtoo worker
callbacks, partly mutated from worker threads, and every frame re-derives
decisions in two places (`ImageRenderer::prepare` and `::draw` both call
`plan_view`, which also advances the scale debouncer).

## Findings (master, 2026-10-07)

Reproduced with 48 JPEGs (2–4.6k px), cold thumtoo cache, tile debug on,
zoom in/out, then idle for 30 s: three images stayed on the blurry LQIP
forever with `pending_requests=0`.

1. **Overview state machine corrupts itself → LQIP forever.**
   `ImageOverview` keeps `m_state`, `m_underlay`, `m_surface` and a queue of
   pending surfaces. A failed levels reply pushes `nullopt` and the LQIP
   lookup pushes a surface right after; `process()` handles the `nullopt`
   first and resets `m_underlay = None`, then uploads the LQIP with
   `m_underlay` still `None`. `ImageRenderer::plan_view` treats
   *Ready + surface + not LQIP* as a sharp underlay and sets `skip_grid`:
   no grid tile is ever requested again for that image.
2. **Levels underlay never works.** All `request_pixels` replies are JXL
   (thumtoo soft ladder); surf has no JXL decoder, every decode throws
   ("unknown file type"): `overview failed=45/48`. Its failure path is what
   triggers (1).
3. **Overview fields written from thumtoo workers.** The `request_pixels`
   callback runs on the Client worker (inline Executor) and writes
   `m_state` / `m_underlay` of `ImageOverview` while the GUI thread reads
   them. Data race; order of (1) depends on it.
4. **Cancel never reaches thumtoo.** `cancel_jobs` / `cleanup` only set a
   `JobHandle` flag; the thumtoo job keeps running and the result is
   dropped on arrival. Zooming through several levels queues a full
   viewport batch per level and the final level waits behind all of them →
   long periods on coarser stand-ins (worst with PDF pages).
5. **Legacy thumtoo API with lossy contract.** Galapix uses
   `Client::request_tiles`, which maps Failed and Unavailable to `nullopt`
   and drops Cancelled cells without any callback. `request_tile_cells`
   (exactly-once, explicit `TileStatus`) and `cancel_tile_cells` exist and
   are unused.
6. **Dead cells.** Failed cells are retried 3× on consecutive frames (no
   backoff), then skipped until the visible rect or scale changes
   (`cancel_jobs` early-outs while the view is still). A transient failure
   during a burst leaves the cell on a stand-in until the user moves.
7. **Grid mismatch with thumtoo.** thumtoo sizes level *s* as
   `floor(w / 2^s)` and its grid as `ceil(that / 256)`; Galapix computes
   columns as `ceil(w / (256·2^s))` and places tiles at exact
   `256·2^s` multiples. When `w mod (256·2^s)` is in `1 … 2^s-1` Galapix asks
   for one column/row too many (thumtoo: Unavailable → dead cell), and tile
   placement is off by up to one level pixel at the edges.
8. **Grid waits for the overview.** `ImageRenderer::prepare` does not mark
   grid cells until the overview has a surface or failed, so on a cold
   open the visible tiles queue behind the (failing) levels request.
9. **Per-image scale debouncer** advanced from both prepare and draw; each
   image holds its own committed scale, so neighbours can sit at different
   scales after a zoom.

## Target architecture

Two parts:

1. **Tiles: `thumtoo::lod`** (thumtoo `docs/TILE_LOD.md`), the core biltoo
   already runs on, written against the same failures as findings 1–9. It
   owns every tile cell (Missing → Queued → Ready / Failed / Unavailable),
   one outstanding request per cell, cancel by withdrawing demand, retry
   with backoff, stall watchdog, LRU byte budget, and the draw plan (exact →
   finest Ready parent → LQIP). Galapix does not reimplement any of it.
2. **Workspace: ECS** (EnTT, vendored as a git subtree in `external/entt`).
   All world state lives in the registry and is mutated on the main thread
   only, by systems in a fixed order. Worker threads never touch it.

### Entities and components

One entity per workspace image.

| Component | Data |
|-----------|------|
| `Source` | URL, thumtoo URI, kind (thumtoo / zoomify / mandelbrot) |
| `Transform` | center pos, scale, angle (layout, tools, selection write this) |
| `ImageSize` | native w×h and min scale; absent while the size probe runs |
| `Tiles` | `std::shared_ptr<lod::TileLoader>` (shared per URI) + `lod::TileSession` |
| `Lqip` | LQIP texture (ThumbHash from `get_lqip`), drawn only where the draw plan says Underlay |
| `Textures` | GPU textures of Ready cells, keyed by `TileKey`, created from the loader's RGBA8 bitmaps and dropped with the cell |
| `Visible` | tag, set by the visibility system |
| `Selected` | tag |

The levels underlay goes away (finding 2): biltoo shows LQIP and tiles
only; the core already demands the two coarser levels first, so a sharp
stand-in arrives before the target scale.

### Frame (systems in order)

1. **SizeProbe** — attach `ImageSize` + create `Tiles` when sizes arrive.
2. **Visibility** — quad-tree query of the view rect → `Visible`.
3. **TileView** — for visible images: `session.set_viewport()` with the
   image's content rect and screen density (cheap, renews the demand lease);
   for images leaving the view: `session.clear_demand()`.
4. **Scheduler** — `lod::TileScheduler::pump()` when its wake hook fired
   (the hook posts an SDL user event / timer; no polling).
5. **Upload** — Ready cells without a texture → texture (budget per frame);
   textures of cells that are no longer Ready are dropped.
6. **Render** — per visible image: LQIP where needed, then
   `session.draw_plan()` commands (content-space dst rect, source UV) into
   the wstdisplay Canvas. Read-only.

Tile debug (`v`) shows `TileSession::Phase`, `status_line()` and per-cell
kind (exact / parent / underlay) from the draw plan.

### Providers

thumtoo images use `lod::ClientTileBackend` (`request_tile_cells`,
results already RGBA8). Zoomify and Mandelbrot implement `lod::TileBackend`
on top of the JobManager (exactly one result per cell, Cancelled included).

### What moves where

| Today | After |
|-------|-------|
| `ImageTileCache`, `ImageRenderer`, `ImageOverview`, `JobHandle` in the tile path | `thumtoo::lod` + Upload/Render systems |
| global request budget, scale debouncer, `cancel_jobs` | gone (scheduler caps, demand lease) |
| `Workspace` (image list, selection, layouter, save/load) | registry + systems; layouters write `Transform` |
| `WorkspaceItem` / `Image` | entity + components |
| `SizeProbeSession` | SizeProbe system |
| `Viewer::current()->redraw()` from workers | scheduler wake / loader change → redraw |
| tools | operate on `Transform` / `Selected` via queries |

ImGui status, input handling, `AppViewer` and the wst renderer stay.

## Migration

One branch, commits that each build and run:

1. Tiles on `thumtoo::lod` behind the existing `Workspace` / `Image` API:
   delete `ImageTileCache`, `ImageRenderer`, `ImageOverview`; fixes findings
   1–9.
2. EnTT subtree; workspace, selection, layouters, save/load on components.
3. Tools and Viewer queries on components; remove `WorkspaceItem`.
