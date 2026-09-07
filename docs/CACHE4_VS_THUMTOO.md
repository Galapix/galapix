<!--
SPDX-FileCopyrightText: 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# cache4 vs thumtoo — feature audit and removal plan

Investigation (2026-09-07) for retiring Galapix’s historical **cache4** tile
stack once thumtoo covers the view path. Related: [THUMTOO.md](THUMTOO.md),
[TILE_LOADING.md](TILE_LOADING.md), [DEPENDENCIES.md](DEPENDENCIES.md).

## Product direction (near → far)

| Horizon | Owner | Role |
|---------|-------|------|
| **Now** | Galapix | Zoomable **viewer** + workspace layout; tiles from **thumtoo** |
| **Near** | thumtoo | Durable pixels, **query/library**, and (growing) **data retrieval**: flexible nested URIs, network + archive + PDF + content-id blob access — not more Galapix resource SQL |
| **Later** | Galapix (+ thumtoo) | Browsable self-contained **dataverse** UI on top of thumtoo. Spec deferred; avoid premature architecture in Galapix |

`-p` / resource-DB listing is **transitional**. New query **and** fetch/URI features belong in **thumtoo** so Galapix, biltoo, and dirtoo share one backend. See [THUMTOO.md](THUMTOO.md) (data retrieval / live PDF tiles).


## What “cache4” is

Galapix opens two SQLite files under the database prefix (`Options::database`):

| File | Role | Pure thumtoo view (`use_thumtoo`) |
|------|------|----------------------------------|
| **`cache4.sqlite3`** | **Resource** index: files, URLs, blobs, images, archives, video stubs; mtime/size (pattern search moved to thumtoo) | **Still opened** (optional metadata; patterns use thumtoo) |
| **`cache4_tiles.sqlite3`** | **Tile** store: 256² JPEG cells keyed by image row + scale + (x,y) | **Not created**; `MemoryTileDatabase` stub only |

`Database::create(prefix)` opens **resource** `cache4.sqlite3` only, plus an
in-memory tile stub. Durable tiles are thumtoo’s. View path for files uses
`ThumtooTileProvider` when `HAVE_THUMTOO`; Zoomify/Mandelbrot stay separate
providers. Galapix SQLite tile generation was removed (Phase 2).

## Feature matrix

| Capability | cache4 tiles + Galapix jobs | thumtoo | Gap / note |
|------------|----------------------------|---------|------------|
| On-demand 256² JPEG cells, scale 0 = full res | Yes (`TileGenerationJob`, `TileGenerator`) | Yes (`request_tile`) | Covered |
| Durable multi-process cache | `cache4_tiles.sqlite3` | `$XDG_CACHE_HOME/thumtoo` | Covered (better shared) |
| Content identity | SHA1 via OpenSSL in resource/tile path | sha256 content id | Covered |
| Progressive coarser stand-in on miss | Galapix `ImageTileCache` + parent requests | Same cache logic + thumtoo single-scale | Covered on Galapix side |
| Fast **overview** (not a grid tile) | Historically mixed into gen; now `ImageOverview` + `TileGenerator::load_surface` (libjpeg scale) | Not tile API; Galapix owns overview | **Keep** overview path; not a tile-DB feature |
| Full-pyramid batch generate | `request_tiles` / multi-scale jobs | `request_tile_pyramid` / `thumtoo-prepare` | Prefer thumtoo CLI |
| Pattern list (`-p`) / known files | **Resource** DB (legacy, stubs) | `list_locators_*` | **Done:** `-p` uses thumtoo cache |
| Image size without opening file | `ImageEntry` in resource DB | `get_size` / `request_size` | thumtoo sufficient on pure view |
| Archive member images | arxp + resource + tiles | libarchive + URI `…//archive:…` | Prefer thumtoo; **arxp** still in tree for `ArchiveThread` |
| PDF pages | Weak / external tools historically | poppler via thumtoo | Prefer thumtoo |
| HTTP(S) / Zoomify | libcurl + `ZoomifyTileProvider` | Not yet | Future: thumtoo network URIs; Zoomify may stay special |
| Nested archive / PDF / content-id URIs | Partial (arxp, PDF expand) | Partial (`//archive:`, `//page:`) | **thumtoo** general retrieval — see THUMTOO.md |
| Live PDF tiles (no pre-raster pyramid) | — | — | **thumtoo** future (mandelbrot-style on-demand) |
| Mandelbrot / builtin | `MandelbrotTileProvider` | N/A | **Keep** as Galapix demo provider |
| Tile cleanup / delete by file id | `SQLiteTileDatabase::delete_tiles` | thumtoo cache tools | Operational difference only |
| File-on-disk tile layout | **`FileTileDatabase`** | — | **Dead code** (never constructed) |

### What is *not* a reason to keep cache4 tiles

* Overview rendering → `ImageOverview` / `TileGenerator::load_surface` (stdio JPEG), independent of tile SQLite.
* Workspace layout save/load → `.galapix` files, not cache4.
* ImGui / tools / status → unrelated.

### What still needs *some* Galapix DB or equivalent

* **`-p` pattern** → thumtoo locator list APIs (cache-only).
* Optional local metadata (mtime checks before regen) → resource tables or replace with thumtoo + filesystem.

## Code surface (tile side)

Rough ownership of the **legacy tile** path (safe to gate or delete once
`--no-thumtoo` is dropped):

| Area | Files (indicative) |
|------|--------------------|
| Tile DB | `sqlite_tile_database.*`, `cached_tile_database.*`, `memory_tile_database.*`, `file_tile_database.*` (**unused**), `tables/tile_table.*`, `statements/tile_entry_*`, `entries/tile_entry.*` |
| Provider | `database_tile_provider.hpp` |
| Thread API | `DatabaseThread::request_tile(s)`, `generate_tiles`, store paths |
| Jobs | `tile_generation_job.*`, `multiple_tile_generation_job.*`, much of `tile_generator.*` (overview still needs `load_surface` / cut helpers) |
| Resource bridge | `ResourceManager::request_tile_info` / `generate_tiles` |

**Resource** side (`cache4.sqlite3`) is larger and **orthogonal**: file/url/blob/image/archive/video tables + statements. Do not delete with tiles.

## Dependency impact

| Dependency | Tied to tiles? | Tied to resource DB? | Notes |
|------------|----------------|----------------------|-------|
| **SQLiteCpp** + **sqlite3** | Yes | Yes | Stays while resource DB stays |
| **OpenSSL** | SHA in tile/resource | Yes | `src/util/sha1.cpp` |
| **libjpeg** / **libpng** | Encode/decode tiles | surfcpp / overview | Stay |
| **ImageMagick** | Broad decode for tile gen | surfcpp formats | Stay until format policy changes |
| **arxpcpp** | Archive extract for old path | `ArchiveThread`, `App` | Separate from tile SQLite; thumtoo has libarchive |
| **libcurl** | Downloads / Zoomify | Network | Stay |
| **libmhash** | — | — | **Unused** (OpenSSL EVP) |
| **jsoncpp** | — | — | **Unused** |
| **EnTT** | — | — | **Linked, unused** |
| **Python** (CMake `find_package`) | — | — | **Unused** |
| thumtoo stack (vips, jxl, poppler, libarchive) | — | — | Required when `WITH_THUMTOO` |

Removing **only** the tile SQLite path does **not** drop SQLiteCpp. Removing the
**resource** DB later would.

## Removal phases (proposed)

Do **not** delete the legacy viewer path in one shot; keep `--no-thumtoo` until
smoke confidence is high.

### Phase 0 — prepare (**done** 2026-09-07)

* Document this matrix (this file).
* Deleted **dead** `FileTileDatabase` (never selected in `Database::create`).
* Dropped unused package deps: **libmhash**, **jsoncpp**, **EnTT**, idle **Python** find.
* Keep `MemoryTileDatabase` while anything still calls `get_tiles()` under pure thumtoo.

### Phase 1 — default-only thumtoo (**done** 2026-09-07)

* `HAVE_THUMTOO` builds **always** use thumtoo tiles; never open `cache4_tiles.sqlite3`.
* `--no-thumtoo` rejected (error); `--thumtoo` kept as no-op for old scripts.
* `DatabaseTileProvider` only compiled into the open path when **not** `HAVE_THUMTOO`.
* Legacy SQLite tile sources remain in the tree for `WITH_THUMTOO=OFF` builds only.

### Phase 2 — remove Galapix tile SQLite (**done** 2026-09-07)

* No `cache4_tiles.sqlite3`; `Database::create(prefix)` only opens resource DB +
  in-memory tile stub (`CachedTileDatabase`/`MemoryTileDatabase`).
* Removed: `SQLiteTileDatabase`, `tile_table`, tile statements,
  `DatabaseTileProvider`, `TileGenerationJob`, `MultipleTileGenerationJob`,
  `DatabaseThread` tile request/generate/receive paths.
* **Kept:** `TileGenerator` (overview / surface helpers), `Tile`/`TileEntry`,
  Zoomify/Mandelbrot providers, thumtoo provider.
* Builds without `HAVE_THUMTOO` can still open Zoomify/Mandelbrot; plain files
  need thumtoo.

### Phase 3 — resource DB slim / replace (thumtoo query)

* **`-p` migrated** to thumtoo locator list APIs. Do not grow Galapix SQL
  patterns further. Tags/collections still belong in thumtoo.
* Resource `cache4.sqlite3` is no longer required for pattern open; keep only
  while other metadata consumers remain.
* Archive: prefer thumtoo URIs; keep arxp only while Galapix `ArchiveThread` remains.
* **Dataverse** (browsable self-contained corpus UI) is a **much later** Galapix
  product layer on top of thumtoo data — not a reason to keep cache4 SQL.

### Phase 4 — dependency wins after Phase 2–3

* No immediate Magick/curl removal from tile retirement alone.
* arxp only if archive UI is thumtoo-only and `ArchiveThread` is gone.

## Recommendation

1. **Tiles:** thumtoo already owns the pure view path; invest in thumtoo gaps
   (single-cell cut, archive coalesce) rather than new cache4 tile features.
2. **Query / library index:** implement in **thumtoo**, not more Galapix SQL.
3. **Resource `cache4.sqlite3`:** `-p` no longer depends on it; keep only while
   other metadata consumers remain (do not remove casually).
4. **Dataverse** vision: defer design; Galapix remains a spatial viewer until then.

## Related code entry points

* `Database::create` — `src/database/database.cpp`
* `ViewerCommand` open path — `src/galapix/viewer_command.cpp`
* `ThumtooTileProvider` — `src/thumtoo/`
* ~~`DatabaseTileProvider`~~ — removed (Phase 2)
* `DatabaseThread` — `src/server/database_thread.*`
