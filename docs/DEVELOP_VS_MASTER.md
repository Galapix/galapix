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

| Command | master | develop | Notes |
|---------|--------|---------|--------|
| `view` | yes | yes | Main viewer path |
| `thumbgen` | yes | yes | Small thumbs / limited generation |
| `list` | yes | yes | List DB files |
| `cleanup` | yes | yes | GC |
| `export` | yes | yes | Export scaled images |
| `info` | — | yes | develop-only |
| **`prepare`** | **yes** | **missing** | Full tile pyramid generation on master |
| **`filegen`** | **yes** | **missing** | File entries only |
| **`check`** | **yes** | **missing** | DB consistency |
| **`merge`** | **yes** | **missing** | Merge databases into `-d` DB |

Usage text on develop also omits `prepare` / `filegen` / `check` / `merge`.
For pure thumtoo view, offline “prepare” may mean **thumtoo prepare** instead of
Galapix `prepare`; parity for **legacy SQLite tiles** still needs a develop
equivalent or an explicit “use thumtoo CLI” policy.

## Features present on develop, absent on master

- **thumtoo** tile integration (default-on when built with `HAVE_THUMTOO`)
- Pure thumtoo mode (no `cache4_tiles.sqlite3`)
- `nix develop` helpers (`galapix-configure` / `build` / `run` / `gdb`)
- AGENTS.md, docs/THUMTOO.md, docs/DEPENDENCIES.md
- Workspace reader/writer via **priocpp**
- Numeric path sort (strut)
- GTK as optional (`BUILD_GALAPIX_GTK` default OFF in CMake; flake may still enable)

## Features on master that are missing, reduced, or moved on develop

### Missing CLI (functional gap)

- **`prepare`** — generate all tile scales for given files (master).
- **`filegen`** — populate file entries without full tiles.
- **`check`** — database consistency checks.
- **`merge`** — merge multiple DB files into one.

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

**No automatic migration.** Users switching branch must use a new cache dir or
write a migrator. Document this in release notes when merging to master.

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

## Merge strategy (develop → master)

1. **Treat develop as the source of truth** for architecture (external libs,
   glad, thumtoo).
2. **Do not** try to reintroduce Boost/GLEW/in-tree display on develop.
3. Before merge:
   - Restore or deliberately obsolete **`prepare` / `filegen` / `check` /
     `merge`** (map `prepare` → thumtoo + optional SQLite tile gen).
   - Document **cache3 → cache4** incompatibility (or provide migration).
   - Decide GTK default for the release package.
   - Run SDL view smoke tests (local JPEG, zoom levels, edge tiles, resize).
   - Align flake package name/binaries (`galapix-0.3.sdl` vs master paths).
4. Merge develop into master (or replace master tip with develop after tag),
   with a clear NEWS entry.

## Related docs

- [THUMTOO.md](THUMTOO.md)
- [DEPENDENCIES.md](DEPENDENCIES.md)
- [AGENTS.md](../AGENTS.md)
- [TODO.md](../TODO.md)
