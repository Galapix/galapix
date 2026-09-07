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
| **Near** | thumtoo | Durable pixels **and** richer **query / library** APIs (patterns, collections, metadata) — do not grow Galapix resource SQL for this |
| **Later** | Galapix (+ thumtoo) | Browsable self-contained **dataverse** (explore a corpus as a place, not only open a file list). Spec deliberately deferred; avoid premature architecture in Galapix |

`-p` / resource-DB listing is **transitional**. New query features belong in **thumtoo** so multiple tools (Galapix, biltoo, dirtoo) share one index.


## What “cache4” is

Galapix opens two SQLite files under the database prefix (`Options::database`):

| File | Role | Pure thumtoo view (`use_thumtoo`) |
|------|------|----------------------------------|
| **`cache4.sqlite3`** | **Resource** index: files, URLs, blobs, images, archives, video stubs; pattern search (`-p`); mtime/size | **Still opened** (patterns / optional metadata) |
| **`cache4_tiles.sqlite3`** | **Tile** store: 256² JPEG cells keyed by image row + scale + (x,y) | **Not created**; `MemoryTileDatabase` stub only |

`Database::create(prefix, sqlite_tiles)` wires:

* `sqlite_tiles=true` → `CachedTileDatabase(SQLiteTileDatabase)` → durable tiles
* `sqlite_tiles=false` → `CachedTileDatabase(MemoryTileDatabase)` → no tile file

View path then chooses a `TileProvider`:

* **thumtoo on** → `ThumtooTileProvider` only (no SQLite tile fallback)
* **thumtoo off / no HAVE_THUMTOO** → `DatabaseTileProvider` → `DatabaseThread::request_tile` → tile gen jobs + tile tables

## Feature matrix

| Capability | cache4 tiles + Galapix jobs | thumtoo | Gap / note |
|------------|----------------------------|---------|------------|
| On-demand 256² JPEG cells, scale 0 = full res | Yes (`TileGenerationJob`, `TileGenerator`) | Yes (`request_tile`) | Covered |
| Durable multi-process cache | `cache4_tiles.sqlite3` | `$XDG_CACHE_HOME/thumtoo` | Covered (better shared) |
| Content identity | SHA1 via OpenSSL in resource/tile path | sha256 content id | Covered |
| Progressive coarser stand-in on miss | Galapix `ImageTileCache` + parent requests | Same cache logic + thumtoo single-scale | Covered on Galapix side |
| Fast **overview** (not a grid tile) | Historically mixed into gen; now `ImageOverview` + `TileGenerator::load_surface` (libjpeg scale) | Not tile API; Galapix owns overview | **Keep** overview path; not a tile-DB feature |
| Full-pyramid batch generate | `request_tiles` / multi-scale jobs | `request_tile_pyramid` / `thumtoo-prepare` | Prefer thumtoo CLI |
| Pattern list (`-p`) / known files | **Resource** DB | Not a substitute | **Keep resource DB** until redesigned |
| Image size without opening file | `ImageEntry` in resource DB | `get_size` / `request_size` | thumtoo sufficient on pure view |
| Archive member images | arxp + resource + tiles | libarchive + URI `…//archive:…` | Prefer thumtoo; **arxp** still in tree for `ArchiveThread` |
| PDF pages | Weak / external tools historically | poppler via thumtoo | Prefer thumtoo |
| HTTP(S) / Zoomify | libcurl + `ZoomifyTileProvider` | Not thumtoo | **Keep** separate providers |
| Mandelbrot / builtin | `MandelbrotTileProvider` | N/A | **Keep** |
| Tile cleanup / delete by file id | `SQLiteTileDatabase::delete_tiles` | thumtoo cache tools | Operational difference only |
| File-on-disk tile layout | **`FileTileDatabase`** | — | **Dead code** (never constructed) |

### What is *not* a reason to keep cache4 tiles

* Overview rendering → `ImageOverview` / `TileGenerator::load_surface` (stdio JPEG), independent of tile SQLite.
* Workspace layout save/load → `.galapix` files, not cache4.
* ImGui / tools / status → unrelated.

### What still needs *some* Galapix DB or equivalent

* **`-p` pattern** and “open from previous index” flows → resource tables.
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

### Phase 1 — default-only thumtoo (**in progress**)

* `use_thumtoo` is the supported interactive tile backend when `HAVE_THUMTOO`.
* `--no-thumtoo` prints a **deprecation** warning; startup notes legacy tile SQLite.
* CI / daily use: build with thumtoo on; do not invest in cache4 tile features.

### Phase 2 — remove Galapix tile SQLite

* Stop creating `cache4_tiles.sqlite3`.
* Remove `SQLiteTileDatabase`, tile tables/statements, `DatabaseTileProvider`,
  tile-only `DatabaseThread` APIs, `TileGenerationJob` / multi-tile jobs.
* **Split** `TileGenerator`: keep overview helpers; drop full pyramid write-to-DB path.
* Simplify `Database::create` to resource DB (+ optional null tile interface).

### Phase 3 — resource DB slim / replace (thumtoo query)

* **Do not** expand Galapix `-p` / SQL patterns. Better library query belongs in
  **thumtoo** (shared by viewers and file tools).
* When thumtoo can list/filter a cache or tree, Galapix becomes a consumer of
  that API; then drop or hollow out `cache4.sqlite3` resource tables.
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
3. **Resource `cache4.sqlite3`:** keep only until thumtoo query replaces `-p`;
   do not remove in the same change set as tile SQLite (Phase 2).
4. **Dataverse** vision: defer design; Galapix remains a spatial viewer until then.

## Related code entry points

* `Database::create` — `src/database/database.cpp`
* `ViewerCommand` open path — `src/galapix/viewer_command.cpp`
* `ThumtooTileProvider` — `src/thumtoo/`
* `DatabaseTileProvider` — `src/galapix/database_tile_provider.hpp`
* `DatabaseThread` — `src/server/database_thread.*`
