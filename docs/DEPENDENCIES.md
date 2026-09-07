<!--
SPDX-FileCopyrightText: 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Dependencies and reduction notes

Status after **thumtoo** tile integration (2026-09). thumtoo replaces the
**tile** cache path only. It does **not** replace Galapix’s resource database,
archive workspace flow, or general image loaders.

Primary packaging is `flake.nix` + `CMakeLists.txt`.

## What thumtoo does and does not replace

| Area | Backend after thumtoo |
|------|------------------------|
| Multi-scale **tiles** (view path) | **thumtoo only** when `HAVE_THUMTOO` (`--no-thumtoo` rejected) |
| Resource / file index | **Removed** (galapix-068); thumtoo locators/content |
| Archive listing when scanning dirs | Still **arxpcpp** (`Filesystem` / `App::archive`) |
| PDF / archive **open** | thumtoo expands `//page:` / `//archive:` |
| HTTP(S) / Zoomify download | Still **libcurl** |
| Broad format decode (XCF, …) | Still **ImageMagick** via surfcpp plugin |

thumtoo brings its own stack when `WITH_THUMTOO=ON`: **sqlite3**, **vips**, **libjxl**,
**libarchive**, **poppler** (PDF pages), and its own SQLite under
`$XDG_CACHE_HOME/thumtoo` (not Galapix’s removed cache4). Flake must still
provide **sqlite** for thumtoo’s `pkg_check_modules(sqlite3)` even though
Galapix no longer links SQLiteCpp.

## Candidates to drop (low risk)

**Done (2026-09-07):** CMake no longer finds **libmhash**, **jsoncpp**, **EnTT**,
or idle **Python**. Hashing remains **OpenSSL EVP** (`src/util/sha1.cpp`).

**Done (galapix-065):** flake `buildInputs` also dropped residual **entt**,
**python3**, and unused **libexif** (no Galapix source refs).

See [CACHE4_VS_THUMTOO.md](CACHE4_VS_THUMTOO.md) for tile vs resource DB removal
phases (SQLiteCpp stays until resource DB goes).

## Optional / already feature-gated

| Dependency | Gate | Notes |
|------------|------|--------|
| **gtest** | `BUILD_TESTS` | Not required for the viewer package. |
| **gbenchmark** | `BUILD_BENCHMARKS` | Flake package may enable; `galapix-configure` defaults OFF. |
| **uitest** | `BUILD_TESTS` | Test-only. |
| **libspnav** | `find_package(spnav)` | SpaceNavigator; soft-optional. |

## Feature-sized removals (need product or code work)

| Dependency | Tied to | To remove it |
|------------|---------|----------------|
| ~~**SQLiteCpp**~~ | Removed galapix-068 | — |
| **arxpcpp** | `Filesystem` archive scan, `App::archive`, `ArchiveThread` (mostly idle) | Prefer thumtoo archive expand on open; then delete arxp wiring. |
| **exspcpp** | Transitive of arxpcpp only | Goes away with arxpcpp. |
| **ImageMagick** / GraphicsMagick | surfcpp `imagemagick` plugin; `Magick::InitializeMagick` | Keep JPEG/PNG via surfcpp; lose Magick-backed formats unless another loader is wired. |
| **libcurl** | `DownloadManager`, remote URLs, Zoomify | Local-file / thumtoo-only builds can drop network. |
| ~~**OpenSSL**~~ | Removed with SHA1 helper (galapix-068) | — |
| **libGLU** | **wstdisplay** still uses `gluBuild2DMipmaps` on some uploads | Galapix tile path already avoids mipmap upload; full removal needs wstdisplay changes. |

## Keep for a normal SDL + thumtoo viewer

| Dependency | Role |
|------------|------|
| **SDL2** | Window / input (product binary `galapix`) |
| **OpenGL / mesa / libGL** | Rendering (glad via wstdisplay) |
| **glm** | Viewer transforms |
| **surfcpp** | Software surfaces, JPEG/PNG, etc. |
| **wstdisplay** | OpenGL window, `GraphicsContext`, textures |
| **geomcpp** | Geometry types |
| **logmich** | Logging |
| **priocpp** | Workspace load/save (`ReaderDocument` / `Writer`) |
| **sexpcpp** / **strutcpp** | Part of the prio/util stack as linked today |
| **tinycmmc** | CMake helpers / version |
| **vips**, **libjxl**, **libarchive**, **poppler** | thumtoo when enabled |

Plus the usual pkg-config “silence” inputs in the flake when `.pc` files still pull them in (e.g. `libsysprof-capture`, `pcre`, …).

## Suggested reduction order

1. ~~flake/CMake unused packages~~ (mhash, jsoncpp, EnTT, python3, libexif).
2. **Dead call graph** (no product behaviour change if careful):
   - `DatabaseThread` is started/joined but **no viewer code posts jobs** to it.
   - `FileEntryGenerationJob` still cuts Galapix tiles into the resource DB — only
     reachable via that idle thread.
   - `CachedTileDatabase` / `MemoryTileDatabase` only back `Database::get_tiles()`
     (no view path readers after Phase 2).
3. Slim open path: with `HAVE_THUMTOO`, skip `get_old_file_entry` and always
   `ThumtooTileProvider` (resource DB becomes optional for view).
4. Optional build flags for Magick, curl, spnav if a minimal viewer is desired.
5. Larger projects: delete resource DB + SQLiteCpp; archive via thumtoo only.

## Related docs

- [THUMTOO.md](THUMTOO.md) — tile backend flags, pure mode, scale range
- [AGENTS.md](../AGENTS.md) — architecture and handoff notes
- [TODO.md](../TODO.md) — integration checklist
