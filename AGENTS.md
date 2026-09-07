<!--
SPDX-FileCopyrightText: 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Agent notes — galapix

## Intent

**galapix** is a zoomable **image collection viewer**: large libraries and
gigapixel images, multi-scale tiles, interactive layout. It is a GUI app
(SDL and optional GTK), not a thumbnail library.

Durable multi-resolution **pixels** for shared use across *too apps belong in
[thumtoo](https://github.com/Grumbel/thumtoo). Galapix owns workspace, OpenGL
rendering, layout tools, and (historically) its own SQLite tile tables.

## Related repositories

| Project | Role |
|---------|------|
| [thumtoo](https://github.com/Grumbel/thumtoo) | Size index + ladder + Phase 4 grid tiles (256² JPEG) |
| [biltoo](https://github.com/Grumbel/biltoo) | Qt viewer; primary thumtoo ladder consumer |
| [dirtoo](https://github.com/Grumbel/dirtoo) | File manager; checksums, archives, Thumbnailer1 client |
| [wstdisplay](https://github.com/WindstilleTeam/wstdisplay) | OpenGL window / GraphicsContext (SDL) |
| [surfcpp](https://github.com/grumbel/surfcpp) | SoftwareSurface, JPEG/PNG load |

## Documentation map

| File | Purpose |
|------|---------|
| [README.md](README.md) | Feature overview, dependencies |
| [TODO.md](TODO.md) | Historical backlog + **thumtoo integration** checklist |
| [docs/THUMTOO.md](docs/THUMTOO.md) | Build flags, CLI, pure mode, threading |
| [docs/DEPENDENCIES.md](docs/DEPENDENCIES.md) | Dep audit / what thumtoo does not replace |
| [docs/DEVELOP_VS_MASTER.md](docs/DEVELOP_VS_MASTER.md) | develop vs master feature gap / merge |
| [docs/TILE_LOADING.md](docs/TILE_LOADING.md) | Viewport-first tile load strategy |
| [docs/STATUS_REPORTING.md](docs/STATUS_REPORTING.md) | Console/HUD status; text library options |
| [NEWS.md](NEWS.md) | Release notes |
| thumtoo [INTEGRATION_GALAPIX.md](https://github.com/Grumbel/thumtoo/blob/master/INTEGRATION_GALAPIX.md) | Library-side mapping |
| thumtoo [TILES.md](https://github.com/Grumbel/thumtoo/blob/master/TILES.md) | Scale/tile conventions |

## Architecture (viewer path)

```
CLI (view / thumbgen / …)
  → ViewerCommand
      → Database (resource SQLite; tile SQLite optional)
      → JobManager / DatabaseThread
      → Workspace + Image(TileProvider)
  → System::launch_viewer (SDL or GTK)
      → Viewer::draw / update
          → ImageTileCache → TileProvider::request_tile
```

**TileProvider** implementations:

| Class | Backend |
|-------|---------|
| `ThumtooTileProvider` | thumtoo `get_tile` / `request_tile` (HAVE_THUMTOO) |
| `DatabaseTileProvider` | Galapix SQLite tiles via DatabaseThread |
| `ZoomifyTileProvider` | Zoomify ImageProperties.xml |
| `MandelbrotTileProvider` | Procedural |

Scale convention matches historical Galapix and thumtoo Phase 4: **scale 0 =
full resolution**, tile size **256**.

## thumtoo integration (status 2026-09-06)

Built with `-DWITH_THUMTOO=ON -DTHUMTOO_DIR=…` (Nix flake enables this via
`fetchFromGitHub` pin). Defines `HAVE_THUMTOO=1`.

| Behaviour | Detail |
|-----------|--------|
| Default | `Options::use_thumtoo` true when `HAVE_THUMTOO` |
| Opt out | `--no-thumtoo` → historical SQLite tiles |
| Cache root | `--thumtoo-cache` or `$XDG_CACHE_HOME/thumtoo` |
| Pure view | No `cache4_tiles.sqlite3`; no `DatabaseTileProvider` fallback |
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

- Window events must reach `wstdisplay::OpenGLWindow::handle_event` so
  **glViewport** updates on resize.
- On `sig_resized`, call `GraphicsContext::set_ortho` and `Viewer::reshape`.
- GL loaded via **glad** (aligned with wstdisplay), not GLEW.
- Edge tiles are padded to 256² with edge-clamped pixels; UV covers content
  only (`maxu`/`maxv`). Upload uses level-0 only (no gluBuild2DMipmaps).
- `ThumtooTileProvider` max_scale must match thumtoo (until image fits in one
  256² tile), not ImageEntry’s ≤8px pyramid depth.
- **Texture arrays / atlas batching** (one draw call for many tiles) is a
  worthwhile future optimization in wstdisplay + ImageRenderer; not required
  for correctness. Prefer fixing seams/filters first.

## thumtoo patches

`patches/thumtoo-request-tile-single-scale.patch` is applied in `flake.nix` via
`applyPatches` so interactive `request_tile` builds only the requested scale
(not a full pyramid). Prefer upstreaming to thumtoo when possible.

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
galapix-run view /tmp/*.jpg --verbose --debug
# galapix-run-gtk …   # GTK frontend
# galapix-run-gdb …   # gdb --args galapix-0.3.sdl
# Override: GALAPIX_BUILD_DIR=… THUMTOO_DIR=… CMAKE_BUILD_TYPE=…

# Manual
cmake -B build -DWITH_THUMTOO=ON -DTHUMTOO_DIR=/path/to/thumtoo …
cmake --build build
```

Packaged run:

```bash
result/bin/galapix-0.3.sdl view /path/to/images… --verbose --debug
# or force SQLite tiles:
result/bin/galapix-0.3.sdl view --no-thumtoo …
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
