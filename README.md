galapix — A zoomable image viewer
=================================

galapix is an interactive image viewer for large collections (tens of
thousands of files) and for very large individual images. You can zoom
from postage-stamp overviews down to full resolution without losing
interactivity. Multi-resolution tiles are requested on demand and
cached; the preferred durable backend is
[thumtoo](https://github.com/Grumbel/thumtoo).

Supported inputs include ordinary image files, many formats via
ImageMagick / surfcpp, archive members (through thumtoo), and URLs
where the build enables network backends.


Features
--------

* Large image collections with continuous pan/zoom
* Extremely large images (multi-resolution tile pyramid)
* Wide format coverage (JPEG, PNG, and formats via ImageMagick /
  optional tools)
* Archive-backed images when thumtoo is enabled
* Background tile loading; the UI stays responsive
* Layout tools (regular, tight, spiral, random) and overlap solving
* Workspace save/load
* Optional SpaceNavigator support (spacenavd)
* Optional ImGui status / help overlay
* Optional thumtoo tile + overview pipeline (`HAVE_THUMTOO`)


Portability
-----------

galapix targets GNU/Linux and other POSIX-like systems. External
helpers for exotic formats and some OS-specific paths may not be
portable. OpenGL is required.


Dependencies
------------

### Core (viewer)

| Dependency | Role |
|------------|------|
| **CMake** ≥ 3.x, **C++20** compiler | Build |
| **SDL2** | Window, input, events |
| **OpenGL** (libGL / Mesa) | Rendering via wstdisplay |
| **GLM** | Math |
| **libjpeg**, **libpng** | Common image codecs |
| **ImageMagick** (Magick++) | Additional formats |
| **libsigc++-2** | Signals |
| **curl** | Optional network URLs (thumtoo / backends) |

### Bundled / flake inputs

Several libraries are pulled as Nix flake inputs or live under
`external/` (ImGui, etc.):

* [wstdisplay](https://github.com/WindstilleTeam/wstdisplay) — OpenGL window / GC
* [surfcpp](https://github.com/grumbel/surfcpp) — software surfaces, codecs
* [geomcpp](https://github.com/grumbel/geomcpp), [logmich](https://github.com/logmich/logmich),
  [priocpp](https://github.com/grumbel/priocpp), [sexpcpp](https://github.com/lispparser/sexp-cpp),
  [strutcpp](https://github.com/grumbel/strutcpp), [babyxml](https://github.com/grumbel/babyxml),
  [tinycmmc](https://github.com/grumbel/tinycmmc)

### Optional

| Dependency | Role |
|------------|------|
| **[thumtoo](https://github.com/Grumbel/thumtoo)** | Durable tile cache, ladder, archives, PDF tiles (`-DWITH_THUMTOO=ON`) |
| **SQLite**, **libvips**, **libjxl** | Required by thumtoo when linked |
| **Poppler** (via thumtoo) | PDF pages when thumtoo is built with Poppler |
| **libspnav** / spacenavd | 3Dconnexion SpaceNavigator |
| **rsvg-convert** (librsvg) | Tool icon SVG → PNG at build time |

Thumbnail **generation** and offline tile preparation live in thumtoo
(`thumtoo-prepare` and related tools). Galapix is the interactive
viewer.


Compilation
-----------

### Recommended: Nix flake

```bash
# One-shot package build
nix build

# Interactive development shell (cmake, deps, helpers)
nix develop

# Inside the shell: incremental build + run
galapix-build
galapix-run [OPTIONS] [FILES]...
```

Override the thumtoo source tree when testing a local checkout:

```bash
nix develop --override-input thumtoo path:/path/to/thumtoo
```

Default CMake flags from the flake include `-DWITH_THUMTOO=ON` and
`-DTHUMTOO_DIR=<flake input>`.

### Manual CMake

```bash
cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DWITH_THUMTOO=ON \
  -DTHUMTOO_DIR=/path/to/thumtoo
cmake --build build -j"$(nproc)"
```

`THUMTOO_DIR` may be a thumtoo **source** tree (`add_subdirectory`) or
an install prefix. Without thumtoo:

```bash
cmake -B build -DWITH_THUMTOO=OFF
cmake --build build -j"$(nproc)"
```

Useful options (see `CMakeLists.txt` / `docs/THUMTOO.md`):

* `-DBUILD_TESTS=ON` — unit tests
* `-DBUILD_BENCHMARKS=ON` — benchmarks
* `-DWITH_THUMTOO=ON|OFF` — thumtoo backend

The historical `./waf` build is **gone**; use CMake or the flake.


Running
-------

```bash
# After nix build:
./result/bin/galapix [OPTIONS] [FILES]...

# Dev shell:
galapix-run [OPTIONS] [FILES]...

# Or the binary in the build directory:
build/galapix [OPTIONS] [FILES]...
```

There is no subcommand. A leading `view` is still accepted for older
scripts.

Useful options:

| Option | Meaning |
|--------|---------|
| `-g`, `--geometry WxH` | Initial window size (default **1024×768**) |
| `-f`, `--fullscreen` | Start fullscreen |
| `-t`, `--threads N` | Worker threads (default 2) |
| `-F`, `--files-from FILE` | Read paths/URLs from a file |
| `-p`, `--pattern PATTERN` | Select from thumtoo cache |
| `--thumtoo-cache DIR` | thumtoo cache root (default `~/.cache/thumtoo`) |
| `-a`, `--anti-aliasing N` | 0, 2, or 4 |
| `-h`, `--help` | Help |
| `-V`, `--version` | Version |

Special URL:

```bash
galapix builtin://mandelbrot
```


Keyboard commands
-----------------

| Key | Function |
|-----|----------|
| F11 | Toggle fullscreen |
| 1 | Regular layout |
| 2 | Tight layout |
| 3 | Random layout |
| 4 | Solve overlaps |
| 5 | Tight layout (alternate) |
| h | Reset zoom and scroll |
| m | Move / resize tools |
| p | Pan / zoom tools |
| n | Shuffle images |
| s / Shift+s | Sort A→Z / Z→A |
| i | Keep only selected images |
| Delete | Remove selected from workspace |
| Home / End | Zoom in / out |
| Page Up / Down | Gamma |
| Shift + mouse | Rotate view |
| Left / Right | Rotate ±90° |
| Up | Reset rotation |
| Escape | Quit |

Additional bindings and the on-screen help/status UI are documented in
`docs/STATUS_REPORTING.md` (ImGui overlay).


SpaceNavigator
--------------

Supported via the open-source [spacenavd](http://spacenav.sourceforge.net/)
daemon when galapix is built with libspnav. The proprietary 3dxsrv
daemon is not supported.


Diagnostics
-----------

Environment variables for development (see `docs/` and source):

* `GALAPIX_OPEN_TIMING=1` — open / tile pending samples
* `GALAPIX_FRAME_TIMING=1` — per-frame pump / prepare / draw timing

Tile preparation and cache maintenance belong to **thumtoo**, not the
viewer.


Development
-----------

* `external/` — convenience / vendored pieces (e.g. ImGui)
* `test/` — automated tests (enable with `-DBUILD_TESTS=ON`)
* `uitest/` — interactive tests
* Session handoff notes: top of [TODO.md](TODO.md)
* Architecture and docs map: [AGENTS.md](AGENTS.md)

Related projects: [thumtoo](https://github.com/Grumbel/thumtoo),
[biltoo](https://github.com/Grumbel/biltoo),
[dirtoo](https://github.com/Grumbel/dirtoo).


License
-------

GNU General Public License v3 (or later). See the source headers and
project license files for details.
