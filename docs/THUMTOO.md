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
`//archive:`). No Galapix SQLite tile fallback (removed Phase 2).

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

## Library query (future, not Galapix SQL)

Galapix `-p` / `cache4.sqlite3` resource patterns are transitional. Richer
listing, filtering, and collection queries should land in **thumtoo** so
Galapix, biltoo, and dirtoo share one backend. Galapix’s long-term “dataverse”
UI is separate and much later; it should consume thumtoo, not revive cache4.

## Data retrieval / flexible URIs (future — thumtoo)

Galapix still has pieces of a **resource / blob / network** path (libcurl,
arxp, `Blob*` helpers, content hashing). The long-term home for that is
**thumtoo as a general data-retrieval layer**, not only a thumbnail cache.

Target capability (design notes, not a Galapix roadmap item):

* **Composable location URIs** that can point at, and nest:
  * local files and directories
  * HTTP(S) (and similar) network resources
  * members inside archives (zip, rar, tar, …)
  * **networked archives** (HTTP → archive → member)
  * pages inside PDFs (including PDF-in-archive, archive-in-archive, …)
  * content-addressed blobs (`sha1:` / `sha256:` once bytes are known)
* **Blob access API**: resolve URI → bytes (or stream), with caching keyed by
  content id where possible; tools share one store.
* Galapix (and biltoo/dirtoo) become **consumers** of that URI/blob API for
  open/list/fetch; do not grow a parallel Galapix-only resource stack.

Until thumtoo owns this, Galapix may keep libcurl/arxp for Zoomify and legacy
archive paths, but new work should prefer extending thumtoo URIs.

**thumtoo-016+:** `parse_location` / `format_location`, nested pipes.
**thumtoo-017+:** `Client::resolve_content_id`, `list_uris_for_content_id`,
`get_meta("sha256:…")` — content-id as URI (cache only). Network fetch and
**thumtoo-018+:** `Client::read_source_bytes` for file/archive (and via content-id).
PDF pages: **thumtoo-019** uses kPdfLayoutDpi (144) for size + on-demand
tile rasterize (live-style). **thumtoo-021+:** optional libcurl (`THUMTOO_HAVE_CURL`) for http(s)
probe / tiles / pixels / `read_source_bytes`. **thumtoo-022:** in-process
HTTP body cache: session RAM + **thumtoo-025** durable `http_bodies` in
`blobs.sqlite` (default 7-day TTL).

## Live / procedural tiles (future — thumtoo)

Today PDF pages are **rasterized once** (e.g. media-box at fixed dpi) then cut
into durable 256² JPEG cells. A later option is **real-time rendering** into
tiles at the requested scale (same idea as Galapix `builtin://mandelbrot`:
no pre-baked pyramid, high resolution on demand).

That belongs in **thumtoo** (provider that fills a tile from a live rasterizer),
not a separate Galapix-only PDF path. Mandelbrot can stay a Galapix demo
provider; production formats should plug into the same thumtoo tile contract.


## Remote http(s) images

`thumtoo_uri_from_url` maps Galapix `http://` / `https://` URLs through to
thumtoo unchanged. Requires thumtoo built with libcurl (`THUMTOO_HAVE_CURL`).
Zoomify remains a separate provider.


## Session handoff (2026-09-07)

See repository [TODO.md](../TODO.md). Galapix tip **galapix-055**; pair with **thumtoo-024+**.

## Locator query (thumtoo-026+)

`Client::list_locators_by_uri_prefix` / `list_locators_by_outer_path_prefix` /
`list_locators_like` — cache-only. Galapix `-p` can migrate here later.
