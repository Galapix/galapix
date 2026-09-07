<!--
SPDX-FileCopyrightText: 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# develop vs master

In-depth comparison of Galapix **`develop`** (this tree) and **`master`**
(`https://github.com/Galapix/galapix`, tip as of the last fetch for this
document). Branches diverged heavily (~630 commits on develop-not-master,
~40 on master-not-develop; ~726 paths differ). They are **not** a fast-forward
apart.

**Priority:** make `develop` releasable and **merge develop → master** (see
[TODO.md](../TODO.md)).

## Executive summary

| | **master** | **develop** |
|--|------------|-------------|
| Role | Last “public” / Nix-packaged line | Active rewrite + thumtoo |
| Build | CMake (recently replaced SCons) | CMake + tinycmmc, richer flake |
| C++ | C++11, **Boost**, **GLEW** | Modern C++, **no Boost**, **glad** |
| Display | In-tree `src/display/*` | **wstdisplay** |
| Image codecs | In-tree `src/plugins/*` | **surfcpp** (+ optional Magick) |
| Archives | In-tree rar/zip/7z plugins + tools | **arxpcpp** |
| Tile cache | SQLite `cache3*_tiles` | SQLite `cache4*_tiles` and/or **thumtoo** |
| Resource DB | SQLite `cache3.sqlite3` | SQLite `cache4.sqlite3` (redesigned schema) |
| DB compatibility | **Incompatible** with develop caches | **Incompatible** with master caches |

develop is ahead on architecture and thumtoo; master still exposes several
**CLI maintenance commands** and an older, self-contained plugin/display stack
that develop no longer ships in-tree.

## Architecture

### master

- Monolithic tree: OpenGL helpers, JPEG/PNG/Magick/KRA/RSVG/archive loaders,
  SQLite wrappers live under `src/`.
- **Boost**: filesystem, system, signals2, format.
- **GLEW** for GL entry points.
- Viewer + SDL/GTK frontends similar in spirit to develop, but older APIs.

### develop

- Split into external libraries: **wstdisplay**, **surfcpp**, **geomcpp**,
  **arxpcpp**, **priocpp**, **logmich**, **strutcpp**, **sexpcpp**, etc.
- **glad** (aligned with wstdisplay); GLEW removed from the link line.
- Optional **thumtoo** tile backend (`HAVE_THUMTOO`, pure mode, GUI callback
  queue). See [THUMTOO.md](THUMTOO.md).
- Resource DB reworked (tables/statements under `src/database/`, SQLiteCpp).

## CLI commands

Galapix on **develop** is the viewer only. There is no subcommand: pass files
or URLs directly (a leading `view` is still accepted for old scripts).

| Command | master | develop | Notes |
|---------|--------|---------|--------|
| *(default / files)* | via `view` | **yes** | Viewer; no subcommand required |
| `view` | yes | compat only | Optional leading token; ignored |
| `thumbgen` | yes | **removed** | Use **thumtoo** / `thumtoo-prepare` |
| `list` | yes | **removed** | thumtoo / resource tooling |
| `cleanup` | yes | **removed** | thumtoo |
| `export` | yes | removed | Was mostly dead (`#if 0`) |
| `info` | — | removed | Develop-only debug stub |
| **`prepare`** | **yes** | **not ported** | Use **thumtoo-prepare**; improve thumtoo UX |
| **`filegen`** | **yes** | **not ported** | Open/view + resource DB / thumtoo size probe |
| **`check`** | **yes** | **not ported** | Optional later; not a merge blocker |
| **`merge`** | **yes** | **not ported** | Optional later; not a merge blocker |

Policy: do **not** reimplement master-only maintenance commands on develop.
Offline tile build is **thumtoo**’s job.

## Features present on develop, absent on master

- **thumtoo** tile integration (default-on when built with `HAVE_THUMTOO`)
- Pure thumtoo mode (no `cache4_tiles.sqlite3`)
- `nix develop` helpers (`galapix-configure` / `build` / `run` / `gdb`)
- AGENTS.md, docs/THUMTOO.md, docs/DEPENDENCIES.md
- Workspace reader/writer via **priocpp**
- Numeric path sort (strut)
- GTK as optional (`BUILD_GALAPIX_GTK` default OFF in CMake; flake may still enable)

## Features on master that are missing, reduced, or moved on develop

### Master-only CLI (intentionally not on develop)

See table above. Prefer improving **thumtoo**’s CLI/UI over Galapix `prepare`.

### Moved out of tree (not necessarily broken)

| master location | develop equivalent |
|-----------------|-------------------|
| `src/display/*` | wstdisplay |
| `src/plugins/jpeg|png|imagemagick|…` | surfcpp (+ Magick when linked) |
| `src/plugins/rar|zip|…` | arxpcpp / libarchive (thumtoo) |
| Boost signals/format/filesystem | std / logmich / strut / custom |

Behaviour depends on those libraries’ coverage (e.g. KRA/XCF/SVG still need
Magick or dedicated loaders in surfcpp).

### Disabled / narrowed on packaging

- **Calligra** support was disabled on master (qtwebkit). develop does not
  resurrect it.
- **GTK**: master CMake default ON; develop CMake default OFF.
- **SpaceNavigator**: optional on both; master Nix/Guix notes differ.

### Database / cache (breaking)

| | master | develop |
|--|--------|---------|
| Resource DB path | `~/.galapix/cache3.sqlite3` | `~/.galapix/cache4.sqlite3` |
| Tile DB path | `~/.galapix/cache3_tiles.sqlite3` | `cache4_tiles.sqlite3` or thumtoo cache |
| Schema | older FileEntry-centric | redesigned resource/blob/image tables |

**No automatic migration required for merge.** Focus on **feature parity** on
develop. Optional later: a **manual** cache3→cache4 conversion tool if users
need old caches. Until then, use a fresh `cache4` / thumtoo directory. Mention
incompatibility in NEWS.

### Possible regressions / soft spots on develop (not “missing on purpose”)

These are areas that needed recent fixes on develop or remain sensitive:

- Tile **max_scale** must match thumtoo’s 256-tile range (not ImageEntry ≤8px).
- Edge tiles: RGBA pitch, edge-clamp pad, no-mipmap upload (black/shear issues).
- Window resize must forward to `OpenGLWindow::handle_event` + `set_ortho`.
- Failed tile jobs should be retriable (`ImageTileCache`).
- Master’s self-contained GLEW/Boost path is gone; downstream packagers must
  follow the flake inputs.

## Build and packaging

| | master | develop |
|--|--------|---------|
| Generator | CMake (≥3.14) | CMake 3.16…3.31, tinycmmc |
| Nix | flake present | flake + **devShell** + thumtoo pin |
| Guix | historically present on master | not primary on develop |
| Tests | `test/` | `test/` + `uitest/` + optional benchmarks |


## Interactive UI (SDL viewer)

Keyboard/tool surface is largely the **same** on both branches (pan/zoom/move,
layouts 1–6, grid, isolate, delete, sort/shuffle, gamma/brightness, trackball,
SpaceNavigator when built). Tools under `src/tools/` match by name.

### Known develop differences / things to test

| Area | master | develop | Risk |
|------|--------|---------|------|
| **F11 fullscreen** | `toggle_fullscreen()` | SDL toggles FullscreenDesktop ↔ Window | Fixed on develop SDL path |
| **Initial layout** | `layout_tight()` + **`finish_layout()`** | ViewerCommand path; no `finish_layout` | Startup framing may differ |
| Window / GL | in-tree `SDLWindow` | **wstdisplay::OpenGLWindow** | Resize, DPI, fullscreen |
| Tile display | SQLite tiles | thumtoo and/or SQLite | Zoom scales, edge tiles |
| GTK | CMake default ON | CMake default OFF | Secondary frontend |
| Workspace F2/F3 | yes | yes (priocpp) | Format compatibility untested |

### Suggested manual UI test list

1. Several local JPEGs with thumtoo; zoom all scales (no missing tiles).
2. Right/bottom image edge: no black rows or shear.
3. Window **resize** (viewport + ortho).
4. **F11** fullscreen and back to windowed (fix/fix).
5. Tools: pan (P), zoom (Z), move/resize (M), rotate (R), grid (Y).
6. Layouts 1–6; sort (S), shuffle (N); isolate (I); Delete.
7. F2/F3 workspace load/save if used.
8. Optional: `builtin://mandelbrot`, archives, GTK build.

## Merge strategy (develop → master)

1. **Treat develop as the source of truth** (external libs, glad, thumtoo).
2. **Do not** reintroduce Boost/GLEW/in-tree display, or Galapix `prepare` /
   `filegen` / `check` / `merge` (thumtoo CLI / UX instead).
3. Before merge:
   - **UI testing** (table above); fix F11 toggle and layout/startup gaps.
   - NEWS: cache3/cache4 incompatible; optional manual converter later.
   - Decide GTK default; align flake binaries.
4. Merge develop → master (or retarget default branch after tag).

## Related docs

- [THUMTOO.md](THUMTOO.md)
- [DEPENDENCIES.md](DEPENDENCIES.md)
- [AGENTS.md](../AGENTS.md)
- [TODO.md](../TODO.md)
