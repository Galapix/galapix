<!--
SPDX-FileCopyrightText: 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

Galapix 0.3.0-dev (unreleased)
==============================

### Architecture

* Modern C++ stack: **no Boost**, **no GLEW** (glad via wstdisplay)
* Display: **wstdisplay** (SDL OpenGL window / GraphicsContext)
* Image codecs: **surfcpp** (+ optional ImageMagick)
* Archives: **arxpcpp** / libarchive (thumtoo for tile members)
* Build: CMake + **flake.nix** (`nix develop` helpers)

### Database / caches (incompatible with 0.1.x)

* Resource DB: `~/.galapix/cache4.sqlite3` (redesigned schema)
* Legacy tile DB: `cache4_tiles.sqlite3` when not using thumtoo
* **No automatic migration** from cache3 / old tile tables — use a fresh
  cache directory. Optional manual conversion may come later; not required
  for the develop → master merge

### thumtoo tile backend

* Default when built with `HAVE_THUMTOO` (`--no-thumtoo` forces SQLite tiles)
* Pure thumtoo view skips Galapix `cache4_tiles.sqlite3`
* Content-addressed tiles (sha256) under `$XDG_CACHE_HOME/thumtoo`
* Interactive `request_tile` limited to the requested scale (in-tree patch;
  full pyramids via `thumtoo-prepare` / `request_tile_pyramid`)
* Batch size probes at view open; tile JPEG decode off the GUI thread;
  budgeted GL uploads per frame
* On tile miss: request max_scale overview + parent cell before the target;
  coarser in-flight jobs are not cancelled as the view scale changes
* Offline prepare: use **thumtoo-prepare** (Galapix `prepare` CLI not ported)

### UI (SDL)

* **GTK frontend abandoned** — flake and default CMake build SDL only
* F11 toggles fullscreen desktop ↔ window
* Key **`l`**: print view + tile backlog to stdout (pending requests/uploads)
* **Dear ImGui** status overlay (F1) — pure OpenGL chrome placeholder

### Still in progress

* In-memory libjpeg-scale **overview** under tiles (EXIF thumb / disk cache still open)
* thumtoo single-cell cut and same-archive request coalesce (upstream)
* On-screen status HUD (needs text rendering)
* Interactive UI parity testing before calling develop “master”


Galapix 0.1.3 (??. Oct 2009)
============================

* some general code cleanup
* mandelbrot set generation support
* faster loading by combining FileEntry and Tile generation
* flexible file format support


Galapix 0.1.2 (25. Oct 2009)
============================

* fixed issue with newly generated images not showing up properly
* added automatic check for libspnav
* added proper configuration variables to build script


Galapix 0.1.1 (24. Oct 2009)
============================

* EXIF support
* Zoomify support
* available thumbnails are now loaded directly at startup, instead of
  dynamically later on
* FileEntry and TileEntry generation has been merged to allow faster
  loading
* image refresh on F5 has been fixed


Galapix 0.1.0 (17. Oct 2009)
============================

* database layout has changed
* switched to boost::thread
* added an incomplete Gtk based GUI
* added OpenGL anti-aliasing (--anti-aliasing N)
* added toggling of the background color
* added a flexible grid overlay tool
* support for SVG via RSVG
* support for RAW images via UFRaw
* support for PNG images via libpng
* support for ImageMagick via Magick++
* support for Gimp/XCF via xcftools
* support for Krita/KRA via koconvert
* support for RGBA tiles
* support for zip archive files
* support for rar archive files
* support for http/ftp via libcurl


Galapix 0.0.3 (12. Sep 2008)
============================

* fixes for gcc 4.3
* added random layout function
* added random shuffle
* added function to create non-overlapping layouts
* scale is smoothly handled on relayout
* added MoveTool to move and scale images
* fixed random halting of the application


Galapix 0.0.2 (4. Sep 2008)
===========================

* initial release
