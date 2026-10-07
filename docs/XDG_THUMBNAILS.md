<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# XDG / Freedesktop thumbnails

**Separate** from thumtoo’s durable ladder, LQIP, and Galapix-style 256² tile
pipeline. Use this when a host (e.g. dirtoo) wants desktop-standard thumbnails
shared with file managers.

## Specs

- [Thumbnail Managing Standard](https://specifications.freedesktop.org/thumbnail-spec/latest/)
- D-Bus: `org.freedesktop.thumbnails.Thumbnailer1` on the **session** bus  
  (path `/org/freedesktop/thumbnails/Thumbnailer1`)

## Cache (always available)

| API | Role |
|-----|------|
| `xdg_file_uri(path)` | Absolute `file://` URI with percent-encoding (MD5 key) |
| `xdg_thumbnail_cache_path(path, flavor)` | `$XDG_CACHE_HOME/thumbnails/<flavor>/<md5>.png` |
| `xdg_thumbnail_lookup` | Fresh hit (`Thumb::MTime` / `Thumb::Size` or mtime fallback) |
| `xdg_thumbnail_remove_cache` | Delete normal/large/x-large/xx-large/fail entries |

Flavors: `normal` (128), `large` (256), `x-large` (512), `xx-large` (1024).

## Thumbnailer1 (optional, `THUMTOO_HAVE_DBUS`)

Built when `pkg-config dbus-1` is found (`-DTHUMTOO_WITH_DBUS=ON`, default).

`XdgThumbnailer`:

1. Returns cache hits without talking to the bus.
2. On miss, `Queue(uris, mimes, flavor, "default", 0)`.
3. Listens for `Ready` / `Error` / `Finished`.
4. `process_events` / `request_wait` for hosts without Qt’s event loop.

Does **not** write into thumtoo’s Store or generate tiles.

## CLI

```bash
thumtoo-xdg-thumb /path/to/image.jpg
thumtoo-xdg-thumb --request --mime image/jpeg /path/to/image.jpg
```

## dirtoo

dirtoo’s `dirtoo-thumbnail` is the Qt-oriented cousin. thumtoo’s module is
Qt-free for embedding in non-Qt hosts and for shared URI/cache rules.

## Feature flag

`thumtoo::feature_dbus()` / `THUMTOO_HAVE_DBUS` in `version.hpp`.
