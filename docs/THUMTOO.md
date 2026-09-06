# thumtoo tile backend (optional)

Galapix can use [thumtoo](https://github.com/Grumbel/thumtoo) as the durable
tile cache instead of (or alongside) its SQLite `tiles` table.

## Build

Nix default package enables thumtoo (`fetchFromGitHub` pin +
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
- Misses call `thumtoo::Client::request_tile` (builds requested scale + coarser).

## CLI prewarm

```bash
thumtoo-prepare --tiles /path/to/images…
```

## Runtime

```bash
# HAVE_THUMTOO builds default to thumtoo for local files:
galapix.sdl view [files…]
galapix.sdl view --thumtoo-cache ~/.cache/thumtoo [files…]

# Force legacy SQLite tiles:
galapix.sdl view --no-thumtoo [files…]
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
