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
| Multi-scale **tiles** (view path) | thumtoo (`HAVE_THUMTOO`) or legacy SQLite tiles (`--no-thumtoo`) |
| Resource / file index | Still **`cache4.sqlite3`** via SQLiteCpp |
| Archive collections in Galapix UI | Still **arxpcpp** (`ArchiveThread`) |
| HTTP(S) / Zoomify download | Still **libcurl** |
| Broad format decode (XCF, …) | Still **ImageMagick** via surfcpp plugin |

thumtoo brings its own stack when `WITH_THUMTOO=ON`: **vips**, **libjxl**,
**libarchive**, **poppler** (PDF pages), and its own SQLite under
`$XDG_CACHE_HOME/thumtoo` (not Galapix’s tile DB).

## Candidates to drop (low risk)

**Done (2026-09-07):** removed unused **libmhash**, **jsoncpp**, **EnTT**, and
idle CMake `find_package(Python)` from the default flake/CMake wiring. Hashing
remains **OpenSSL EVP** (`src/util/sha1.cpp`).

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
| **SQLiteCpp** (+ **sqlite3**) | Resource DB: file/image entries, `-p` patterns, workspace metadata | Redesign or drop `cache4.sqlite3`. Pure thumtoo only skips **`cache4_tiles.sqlite3`**. |
| **arxpcpp** | Archive listing/extraction in the Galapix workspace | Drop archive collections or reimplement on thumtoo/libarchive. |
| **exspcpp** | Not used by Galapix | Transitive **arxpcpp** flake input only; goes away with arxpcpp. |
| **ImageMagick** / GraphicsMagick | surfcpp `imagemagick` plugin; `Magick::InitializeMagick` | Keep JPEG/PNG via surfcpp; lose Magick-backed formats unless another loader is wired. |
| **libcurl** | `DownloadManager`, remote URLs, Zoomify | Local-file / thumtoo-only builds can drop network. |
| **OpenSSL** | `SHA1::from_*` for resource/blob identity | Replace with another SHA-1 (or share thumtoo hashing). |
| **libGLU** | **wstdisplay** still uses `gluBuild2DMipmaps` on some uploads | Galapix tile path already avoids mipmap upload; full removal needs wstdisplay changes. |
| **libexif** | No direct Galapix references | Likely residual or surfcpp-related; verify before dropping. |

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
| **sqlite** + **SQLiteCpp** | Resource DB (until replaced) |
| **vips**, **libjxl**, **libarchive**, **poppler** | thumtoo when enabled |

Plus the usual pkg-config “silence” inputs in the flake when `.pc` files still pull them in (e.g. `libsysprof-capture`, `pcre`, …).

## Suggested reduction order

1. Remove **libmhash**, **jsoncpp**, **EnTT**, **python3** from flake/CMake; rebuild.
3. Optional build flags for Magick, curl, spnav if a minimal viewer is desired.
4. Larger projects: optional resource DB, archive via thumtoo, hash without OpenSSL.

## Related docs

- [THUMTOO.md](THUMTOO.md) — tile backend flags, pure mode, scale range
- [AGENTS.md](../AGENTS.md) — architecture and handoff notes
- [TODO.md](../TODO.md) — integration checklist
