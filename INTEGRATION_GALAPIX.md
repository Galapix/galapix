<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Galapix integration

Map Galapix develop’s tile pipeline onto thumtoo Phase 4 tiles. thumtoo remains
a **cache library** (no OpenGL / layout). Galapix keeps rendering and workspace.

## Mapping

| Galapix | thumtoo |
|---------|---------|
| `files` row (url, w, h) | `locators` + `content` (`get_size` / `get_meta`) |
| `tiles` (fileid, scale, x, y) → JPEG | `get_tile` / `request_tile` → `TileBlob` (JPEG) |
| `TileGenerator` | worker `EnsureTiles` + `build_tile_pyramid` |
| `thumbgen` / prepare | `thumtoo-prepare --tiles` (+ optional `--ladder EDGE` preview) |
| Archive `file://…//rar:…` | `file:///…//archive:member` |


## Pixel layers for Galapix

Recommended durable cache (prepare / interactive):

1. **Size** — `request_size` / probe (always).
2. **One small preview** — `request_pixels(uri, 256)` (or 128); single JXL, not a full ladder.
3. **Tiles** — `request_tile` / `thumtoo-prepare --tiles` up to a max scale useful for zoom.
4. **Original** — Galapix decodes from source past max cached scale; do not store full-res in thumtoo.

Do **not** prewarm a multi-edge JXL ladder for Galapix; that duplicated tile data and was slow (`--ladder` encodes one edge only as of thumtoo-007).

Scale convention matches Galapix: **scale 0 = full resolution**, tile size **256**.

## Adapter on Galapix develop

Galapix develop ships an optional **`ThumtooTileProvider`** (`TileProvider`):

- Sources: `src/thumtoo/thumtoo_tile_provider.{hpp,cpp}`
- Build: `-DWITH_THUMTOO=ON -DTHUMTOO_DIR=/path/to/thumtoo`
- Docs: galapix `docs/THUMTOO.md`

```cpp
auto client = thumtoo::Client::open(cache_root);
auto provider = galapix::ThumtooTileProvider::create(
    client, thumtoo::file_uri_from_path(abs_path));
image.set_tile_provider(provider);
```

Cache hits use `get_tile` (JPEG for durable cells). Misses use `request_tile`.

**PDF interactive reply is `codec=rgb888` (raw RGB888), not JPEG.** Raster
images remain JPEG. Galapix must branch on `TileBlob::codec`:

* `"jpeg"` / default → `surf::jpeg::load_from_mem` (unchanged)
* `"rgb888"` → bind `bytes` as tightly packed RGB, size `width×height` from the
  blob (no decode). Required for live PDF cells and all scales finer than
  `kPdfMinDurableTileScale` (−2).

With `HAVE_THUMTOO`, Galapix defaults to thumtoo tiles (`--no-thumtoo` forces SQLite).

### PDF negative scale

Layout size is `kPdfLayoutDpi` (144). Scale 0 = that size. Scale −1 = 2×
pixels / 288 dpi, etc. Request `scale < 0` when the user zooms past 1:1 on a
PDF page. `get_tile_coverage` only reports **stored** scales; it does not
advertise live-only negative scales — the viewer chooses them from zoom level.

## Viewer flow

1. Convert path / archive member to thumtoo URI.
2. `get_size` / `request_size` for layout bounds (also done inside `create`).
3. Visible tiles via `TileProvider::request_tile` → thumtoo.
4. Optional prewarm: `thumtoo-prepare --tiles` or `request_tile_pyramid`.

## Out of scope (first cut)

- PDF / video tiles
- Full replacement of `SQLiteTileDatabase` for all resources
- Remote HTTP tile sources

See [TILES.md](TILES.md) for storage and API details.
