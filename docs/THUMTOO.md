# thumtoo tile backend (optional)

Galapix can use [thumtoo](https://github.com/Grumbel/thumtoo) as the durable
tile cache instead of (or alongside) its SQLite `tiles` table.

## Build

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

## Status

Adapter is library-side only; the main galapix UI still constructs
`DatabaseTileProvider` by default. Wire `ThumtooTileProvider` where images are
created from local paths when `HAVE_THUMTOO` is set.
