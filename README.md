<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# thumtoo

**thumtoo** is a library that remembers what your images (and related media)
look like so apps do not have to re-measure and re-decode the same files on
every open.

It stores native dimensions, multi-resolution **preview tiles**, archive
tables of contents, and optional tags keyed by content hash. Source files are
never modified; everything lives under the usual XDG cache and data directories.

thumtoo is **not** an image viewer or a file manager. Applications such as
[biltoo](https://github.com/Grumbel/biltoo) use it as a shared index and pixel
cache.

## What it provides

| Capability | Notes |
|------------|--------|
| Image size and basic media metadata | Width, height, format; page counts for documents where supported |
| Preview tiles | Zoomable 256² tiles and soft overviews for fast browsing |
| PreferCache / TileSynth | Whole-frame samples from durable tiles when a pyramid exists |
| Page text layers | Native extract (MuPDF / DjVu) and optional **Tesseract OCR**; DPI-aware OCR |
| Archives | Cached member lists for zip, tar, and similar containers (read-only) |
| Tags and sets | Optional user data keyed by content hash (survives cache rebuilds) |
| Directory snapshots | Optional cached folder listings for faster reopen (e.g. slow disks) |
| Purge helpers | `purge_uri` / `purge_path` drop size, pixels, tiles, and cached text layers |

Optional backends (pkg-config): MuPDF, DjVu, libunarr, curl, **tesseract** (+ **leptonica** for `lept.pc`), **dbus** (+ **systemd** for `libsystemd.pc` on Nix). `lib.mkBuildInputs` lists the full set so configure does not spam missing `.pc` files.

## Related applications

| Project | Role |
|---------|------|
| [biltoo](https://github.com/Grumbel/biltoo) | Desktop image viewer (primary consumer) |
| [dirtoo](https://github.com/Grumbel/dirtoo) | Media-oriented file manager |
| [galapix](https://github.com/Galapix/galapix) | Zoomable collection viewer |

## Install and build

**Nix** (recommended toolchain):

```bash
nix develop          # deps + helpers
nix build            # package
nix run .#status -- --help
```

**CMake** (needs libvips, libjxl, SQLite, libarchive; optional MuPDF, DjVu, curl,
tesseract/leptonica, dbus):

```bash
cmake -B build && cmake --build build
ctest --test-dir build
./build/thumtoo-status --version
```

Prefer `nix develop` so `PKG_CONFIG_PATH` includes private deps of optional
modules (e.g. `lept` for tesseract, `libsystemd` for dbus). Version comes from
the top-level `VERSION` file. Packaging may append a revision suffix. Hosts can
call `thumtoo::version_string()` after including `<thumtoo/version.hpp>`.

## Using the cache

By default the **Store** (index, tiles, previews) is under `$XDG_CACHE_HOME/thumtoo/`
(or `~/.cache/thumtoo/`). **User overlays** (`user.sqlite` tags/collections) live
under `$XDG_STATE_HOME/thumtoo/` (or `~/.local/state/thumtoo/`) so a cache wipe
does not drop them.
when the host sets a data root.

Useful tools after install:

```bash
thumtoo-status --cache ~/.cache/thumtoo
thumtoo-prepare --help
thumtoo-gc --cache ~/.cache/thumtoo --store-summary
```

## Manual pages

| Page | Topic |
|------|--------|
| **thumtoo(7)** | Library overview, Store layout, environment variables |
| thumtoo-prepare(1), thumtoo-status(1), … | CLI tools under `man/` |

```bash
man 7 thumtoo
man thumtoo-prepare
```

## Documentation for integrators

| Doc | Topic |
|-----|--------|
| [INTEGRATION.md](INTEGRATION.md) | API mapping for biltoo-style hosts |
| [docs/HOST_CUTOVER.md](docs/HOST_CUTOVER.md) | Store-only client expectations |
| [TILES.md](TILES.md) | Tile pyramid behaviour |
| [docs/TILE_LOD.md](docs/TILE_LOD.md) | Host tile LOD core (`thumtoo::lod`): planner, cell state machine, scheduler |
| [docs/DATABASE.md](docs/DATABASE.md) | On-disk schema overview |
| [TAGS.md](TAGS.md) | Content-hash tags |

## License

GPL-3.0-or-later. See [LICENSES/](LICENSES/) and [REUSE.toml](REUSE.toml).
