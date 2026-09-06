# thumtoo tile backend (optional)

Galapix can use [thumtoo](https://github.com/Grumbel/thumtoo) as the durable
tile cache instead of (or alongside) its SQLite `tiles` table.

## Build

Nix (develop / default package) enables thumtoo via flake input and
`-DWITH_THUMTOO=ON -DTHUMTOO_DIR=…`.

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
galapix.sdl --thumtoo [files…]
galapix.sdl --thumtoo --thumtoo-cache ~/.cache/thumtoo [files…]
```

When `--thumtoo` is set (and `HAVE_THUMTOO` was enabled at build time),
`ViewerCommand` opens a shared `thumtoo::Client` and uses `ThumtooTileProvider`
for local files and archive members (Galapix `//rar:` / `//zip:` → thumtoo
`//archive:`). Falls back to `DatabaseTileProvider` if size probe fails.

Without `--thumtoo`, behaviour is unchanged (SQLite tiles).

## Status

- [x] `ThumtooTileProvider` + URI conversion
- [x] `--thumtoo` / `--thumtoo-cache` CLI
- [x] ViewerCommand image-open path
- [ ] Pure thumtoo mode (no Galapix SQLite tiles at all)
