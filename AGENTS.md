<!--
SPDX-FileCopyrightText: 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Agent notes — galapix

## Intent

**galapix** is a zoomable **image collection viewer**: large libraries and
gigapixel images, multi-scale tiles, interactive layout. It is a GUI app
(SDL viewer), not a thumbnail library.

Durable multi-resolution **pixels** for shared use across *too apps belong in
[thumtoo](https://github.com/Grumbel/thumtoo). Galapix owns workspace, OpenGL
rendering, layout tools, and (historically) its own SQLite tile tables.

## Related repositories

| Project | Role |
|---------|------|
| [thumtoo](https://github.com/Grumbel/thumtoo) | Size index + ladder + Phase 4 grid tiles (256² JPEG) |
| [biltoo](https://github.com/Grumbel/biltoo) | Qt viewer; primary thumtoo ladder consumer |
| [dirtoo](https://github.com/Grumbel/dirtoo) | File manager; checksums, archives, Thumbnailer1 client |
| [wst](https://github.com/WindstilleTeam/wst) | `wstdisplay` module: OpenGL window, Device, Renderer, Canvas (SDL) |
| [surfcpp](https://github.com/grumbel/surfcpp) | SoftwareSurface, JPEG/PNG load |

## Handoff

Latest session notes and tip bundles: top of [TODO.md](TODO.md).

## Documentation map

| File | Purpose |
|------|---------|
| [README.md](README.md) | Feature overview, dependencies |
| [TODO.md](TODO.md) | Historical backlog + **thumtoo integration** checklist |
| [docs/THUMTOO.md](docs/THUMTOO.md) | Build flags, CLI, pure mode, threading |
| [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md) | Dep audit / what thumtoo does not replace |
| [docs/DEVELOP_VS_MASTER.md](docs/DEVELOP_VS_MASTER.md) | develop vs master feature gap / merge |
| [docs/TILE_LOADING.md](docs/TILE_LOADING.md) | Viewport-first tile load strategy |
| [docs/CACHE4_VS_THUMTOO.md](docs/CACHE4_VS_THUMTOO.md) | cache4 tiles/resource vs thumtoo; removal phases |
| [docs/THUMTOO.md](docs/THUMTOO.md) | thumtoo flags; **future** URI/blob retrieval + live PDF tiles |
| [docs/STATUS_REPORTING.md](docs/STATUS_REPORTING.md) | Console + ImGui status overlay (F1) |
| [docs/OVERVIEW.md](docs/OVERVIEW.md) | Fast overview layer (≠ grid tiles) |
| [NEWS.md](NEWS.md) | Release notes |
| thumtoo [INTEGRATION_GALAPIX.md](https://github.com/Grumbel/thumtoo/blob/master/INTEGRATION_GALAPIX.md) | Library-side mapping |
| thumtoo [TILES.md](https://github.com/Grumbel/thumtoo/blob/master/TILES.md) | Scale/tile conventions |

## Architecture (viewer path)

```
CLI (files / http(s) URLs; optional leading "view")
  → ViewerCommand
      → Database (resource cache4.sqlite3 only; no cache4_tiles)
      → thumtoo::Client when HAVE_THUMTOO
      → Workspace + Image(TileProvider)
  → System::launch_viewer (SDL) + ImGui Status/Help
      → ImageTileCache → TileProvider::request_tile
```

**Tip bundles:** galapix-055 + thumtoo-024 — see [TODO.md](TODO.md) session handoff.

**TileProvider** implementations:

| Class | Backend |
|-------|---------|
| `ThumtooTileProvider` | thumtoo `get_tile` / `request_tile` (HAVE_THUMTOO) |
| `ZoomifyTileProvider` | Zoomify ImageProperties.xml |
| `MandelbrotTileProvider` | Procedural |

Scale convention matches historical Galapix and thumtoo Phase 4: **scale 0 =
full resolution**, tile size **256**.

## Session handoff (2026-09-07)

Continue from **galapix-035** + **thumtoo-006** bundles (or newer). See TODO.md
“Session handoff”. Thumtoo is vendored as a git subtree in `external/thumtoo`
(see “Vendored libraries”). ImGui overlay is F1; status also key `l`.
CLI has no subcommands (viewer is default).

## thumtoo integration (status 2026-09-07)

Built with `-DWITH_THUMTOO=ON` (`THUMTOO_DIR` defaults to the vendored
`external/thumtoo`; the Nix flake enables this). Defines `HAVE_THUMTOO=1`.

| Behaviour | Detail |
|-----------|--------|
| Default | `Options::use_thumtoo` true when `HAVE_THUMTOO` |
| Opt out | `--no-thumtoo` → historical SQLite tiles |
| Cache root | `--thumtoo-cache` or `$XDG_CACHE_HOME/thumtoo` |
| Pure view | No `cache4_tiles.sqlite3`; file tiles via thumtoo only |
| Threading | `ThumtooCallbackQueue` + Executor; pumped in `Viewer::draw` |
| Size probe | `request_size` + `drain` + `pump` before provider construct |

Sources under `src/thumtoo/`:

- `thumtoo_tile_provider.{hpp,cpp}` — `TileProvider` adapter
- `thumtoo_uri.{hpp,cpp}` — Galapix URL → thumtoo Location URI
- `thumtoo_callback_queue.{hpp,cpp}` — main-thread callback marshal

Do **not** call thumtoo completion callbacks on the worker thread without
posting through the queue (or an equivalent GUI Executor).

`surf::SoftwareSurface` is **not** contextual-bool; use `std::optional` (or
size checks) after decode.

## OpenGL / SDL notes

- wstdisplay (from [wst](https://github.com/WindstilleTeam/wst)) records
  draws into a `wstdisplay::Canvas`; `Viewer::draw` renders it with the
  view transform as `RenderPass::world_matrix` (grid in `Space::Screen`).
  A frame is `Renderer::begin_frame` → `Viewer::draw` → `end_frame` →
  ImGui → `swap_buffers` (`AppViewer::draw_frame`).
- GPU resources belong to the window's `wstdisplay::Device`. Tiles and
  overviews are `TextureSurfacePtr` (owns a `Unique<Texture>` + `Surface`);
  uploads take a `Device&` (`prepare_tiles` → `process_queue`). Release them
  before the window goes away (`~AppViewer` clears the cache).
- Window events must reach `wstdisplay::OpenGLWindow::handle_event`; on
  `sig_resized` call `Viewer::reshape`.
- wstdisplay keeps its glad loader private; the few raw GL calls in Galapix
  (ImGui icons) use `<GL/gl.h>`.
- Edge tiles are padded to 256² with edge-clamped pixels; UV covers content
  only (`maxu`/`maxv`). Upload uses level-0 only (no gluBuild2DMipmaps).
- `ThumtooTileProvider` max_scale must match thumtoo (until image fits in one
  256² tile), not ImageEntry’s ≤8px pyramid depth.
- **Texture arrays / atlas batching** (one draw call for many tiles) is a
  worthwhile future optimization in wstdisplay + ImageRenderer; not required
  for correctness. Prefer fixing seams/filters first.

## Vendored libraries

All libraries are **git subtrees** (squashed) in `external/`, built with
CMake `add_subdirectory()` (see `build_dependencies()` in CMakeLists.txt).
The flake only has `nixpkgs` and `flake-utils` as inputs; thumtoo's system
dependencies come from `external/thumtoo/flake.nix` (`lib.mkBuildInputs`).

| Prefix | Upstream |
|--------|----------|
| `external/tinycmmc` | https://github.com/grumbel/tinycmmc.git |
| `external/geomcpp` | https://github.com/grumbel/geomcpp.git |
| `external/logmich` | https://github.com/logmich/logmich.git |
| `external/sexp-cpp` | https://github.com/lispparser/sexp-cpp.git |
| `external/priocpp` | https://github.com/grumbel/priocpp.git |
| `external/strutcpp` | https://github.com/grumbel/strutcpp.git |
| `external/xdgcpp` | https://github.com/Grumbel/xdgcpp.git |
| `external/babyxml` | https://github.com/grumbel/babyxml.git |
| `external/surfcpp` | https://github.com/grumbel/surfcpp.git |
| `external/uitest` | https://github.com/grumbel/uitest.git |
| `external/wst` | https://github.com/WindstilleTeam/wst.git |
| `external/thumtoo` | https://github.com/Grumbel/thumtoo.git |

`external/imgui` is a plain copy (see its README.galapix.md).

Update one: `git subtree pull --prefix external/<name> <url> master --squash`.
Changes needed by Galapix are made in the subtree and upstreamed in batches
(`git subtree push` or a topic PR); do not carry `patches/*.patch` +
`pkgs.applyPatches`. Local changes not yet upstream:

- `external/priocpp`: `PRIO_INSTALL` option (no install/export rules as a
  subdirectory, like logmich/sexp-cpp/surf).

**Local thumtoo while developing both:** `galapix-run` / `galapix-configure`
resolve `THUMTOO_DIR` as (1) env `THUMTOO_DIR`, (2) sibling
`$GALAPIX_SOURCE/../thumtoo`, (3) vendored `external/thumtoo`. If the CMake
cache still points at another tree, `galapix-build` reconfigures
automatically. Printout: `galapix-run: THUMTOO_DIR=…` on stderr.

Interactive `request_tile` only builds the requested scale (upstream thumtoo
as of the single-scale change). Full pyramids: `request_tile_pyramid` /
`thumtoo-prepare`.

## Version

`VERSION` holds the base (e.g. `0.3.0-dev`). CMake and the flake append
`.${revCount}+g${shortHash}` for `-dev` builds (same scheme as biltoo). The
string is written to generated `galapix/version.hpp` (`GALAPIX_VERSION_STRING`)
so only TUs that include it rebuild on version change. Nix packages pass
`-DPROJECT_VERSION_FULL=…`.

## Build

```bash
# Nix package (thumtoo enabled in flake)
nix build

# Dev shell (out-of-tree Debug + helpers)
nix develop
galapix-configure   # cmake -G Ninja -DWITH_THUMTOO=ON …
galapix-build
galapix-run /tmp/*.jpg --verbose --debug
# galapix-run-gdb …   # gdb --args galapix
# Override: GALAPIX_BUILD_DIR=… THUMTOO_DIR=… CMAKE_BUILD_TYPE=…

# Manual
cmake -B build -DWITH_THUMTOO=ON -DTHUMTOO_DIR=/path/to/thumtoo …
cmake --build build
```

Packaged run:

```bash
result/bin/galapix /path/to/images… --verbose --debug
# or force SQLite tiles:
result/bin/galapix --no-thumtoo …
```

## Coding conventions

- Prefer small, task-focused commits; no drive-by refactors.
- New files: project license (GPL-3.0-or-later); short copyright header
  consistent with existing Galapix sources (or SPDX where already used).
- Author: Ingo Ruhnke \<grumbel@gmail.com\>
- Agent co-author trailer when applicable: `Co-authored-by: Grok <grok@x.ai>`
- Document ongoing work in `TODO.md` / `docs/THUMTOO.md` so a interrupted
  session can resume from the tree alone.
- No feature removal or silent behaviour change without discussion.
- Avoid hacks; if complexity grows, call out a refactor first.

## Handoff / git bundles

Work may be exchanged as numbered **git bundles** (`galapix-001`, …) with full
history and `HEAD` as ref. Each bundle stacks on the previous tip. Do not reuse
bundle numbers after resets.

## Commits

Author: Ingo Ruhnke \<grumbel@gmail.com\>  
Co-authored-by: Grok \<grok@x.ai\> when applicable  
