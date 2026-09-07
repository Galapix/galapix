# thumtoo tile backend (optional)

Galapix can use [thumtoo](https://github.com/Grumbel/thumtoo) as the durable
tile cache instead of (or alongside) its SQLite `tiles` table.

## Build

Nix default package enables thumtoo (flake input `thumtoo` +
`-DWITH_THUMTOO=ON -DTHUMTOO_DIR=…`).

Manual CMake:


```bash
cmake -B build \
  -DWITH_THUMTOO=ON \
  -DTHUMTOO_DIR=/path/to/thumtoo \
  …
cmake --build build
```

`THUMTOO_DIR` may be a thumtoo **source** tree (add_subdirectory) or an install
prefix (`include/thumtoo/…`, `lib/libthumtoo.a`).

Defines `HAVE_THUMTOO=1` on `libgalapix`.

## Dependency / local forks

The Nix flake pulls **thumtoo source** as a locked flake input (`flake =
false`) and passes it as `THUMTOO_DIR`.

If you must change thumtoo for Galapix before upstream accepts the work, use
**`git subtree`** (see AGENTS.md — thumtoo dependency policy). Do not grow a
pile of `patches/thumtoo-*.patch` files applied via `pkgs.applyPatches`.
Upstream in batches, then switch back to the flake input.

## API

`galapix::ThumtooTileProvider` implements `TileProvider`:

```cpp
auto client = thumtoo::Client::open(cache_root); // Executor as needed
auto provider = galapix::ThumtooTileProvider::create(
    client, thumtoo::file_uri_from_path("/abs/path.jpg"));
image.set_tile_provider(provider);
```

- Tile size **256**, scale **0 = full resolution** (same as historical Galapix).
- JPEG tiles from thumtoo are decoded with `surf::jpeg::load_from_mem`.
- Misses call `thumtoo::Client::request_tile` (requested scale only; use
  `request_tile_pyramid` / `thumtoo-prepare` for multi-scale batch).

## CLI prewarm

```bash
thumtoo-prepare --tiles /path/to/images…
```

## Runtime

```bash
# HAVE_THUMTOO builds default to thumtoo for local files:
galapix [files…]
galapix --thumtoo-cache ~/.cache/thumtoo [files…]

# Force legacy SQLite tiles:
galapix --no-thumtoo [files…]
```

When thumtoo is enabled (`HAVE_THUMTOO` and not `--no-thumtoo`),
`ViewerCommand` opens a shared `thumtoo::Client` and uses `ThumtooTileProvider`
for local files and archive members (Galapix `//rar:` / `//zip:` → thumtoo
`//archive:`). Falls back to `DatabaseTileProvider` if size probe fails.

With `--no-thumtoo` (or a build without `HAVE_THUMTOO`), behaviour is the
historical SQLite tile path.

## Status

- [x] `ThumtooTileProvider` + URI conversion
- [x] `--thumtoo` / `--thumtoo-cache` CLI
- [x] ViewerCommand image-open path
- [x] Default-on when `HAVE_THUMTOO` (`--no-thumtoo` to disable)
- [x] Pure thumtoo view: memory-only Galapix tile DB; no SQLite tile fallback

## Threading

`thumtoo::Client` is opened with a Galapix `Executor` that posts completion
callbacks onto `ThumtooCallbackQueue`. `Viewer::draw` pumps the queue on the
GUI thread so JPEG decode delivery and `ImageTileCache::receive_tile` run
there (same model as biltoo’s Qt queued connection). Size probes during
image open call `drain()` then `pump()` so layout size is available before
the viewer starts.

## Scale range

thumtoo only builds scales until the image fits in **one 256×256 tile**
(`while (w > 256 || h > 256) shrink`). Galapix `ThumtooTileProvider::get_max_scale()`
must use that same range. The historical ImageEntry formula (halve until edge ≤ 8)
requests scales thumtoo will never produce (`min_scale > computed_max` → empty blob).

## Session note (2026-09-07)

Prefer **upstream thumtoo** at or after the multi-worker / extract-cache /
stats commits (bundles `thumtoo-004`…`thumtoo-006`). After updating the
input:

```bash
nix flake lock --update-input thumtoo
```

Prepare performance: `thumtoo-prepare --tiles --stats --jobs N` — see thumtoo
`--help`. Interactive Galapix still uses `request_tile` (single scale) +
in-viewer overview layer for stdio files.


## PDF and archives

With thumtoo enabled, opening a **PDF** expands to one image per page
(`file:///doc.pdf//page:N`, 1-based, capped at 512 pages). **Archives**
expand to one image per image member (`//archive:member`).

Size probes, ladder previews, and **grid tiles** for PDF pages are handled by
thumtoo (rasterize at reported 72 dpi media-box size, then cut 256² JPEG
cells). Requires a thumtoo build that includes PDF tile encode (no longer
stubs `request_tile` for `//page:N`). Until that lands on thumtoo master,
override the flake input to a checkout that has the fix.

MIME types in `galapix.desktop` align with `thumtoo::media_mime_types()` plus
Galapix-only types (SVG, XCF, workspace).
