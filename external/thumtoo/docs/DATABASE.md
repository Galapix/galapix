<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Database redesign — schema

Status: **implemented** in `Store` (index + bulk + user). Client is
**Store-only** (≥262); the C++ `Database` / `BlobStore` classes are **deleted**
(≥272). See [API_MIGRATION.md](API_MIGRATION.md), [HOST_CUTOVER.md](HOST_CUTOVER.md).

**No migration** of pre-redesign ladder rows into tiles: on detecting a
pre-100 epoch, index/bulk are replaced. **User** data (`user.sqlite`) must
survive. Store files live at the cache root (`index.sqlite`, `bulk.sqlite`);
classic dual-path trees are parked under `legacy/` on open.

From schema **100** onward, prefer **in-place additive migration** (e.g. 100→101)
over wiping the cache. Do not treat a normal upgrade as “delete the cache.”
See [DIRTOO_BACKBONE.md](DIRTOO_BACKBONE.md). Current index/user baseline includes
schema **101** (richer directory columns; optional `uuid` on tag/collection defs).

**Scope:** identity, locators, archives, media, tiles, directory cache, tags,
collections, link edges, extracted document structure.  

**Out of scope here (→ §12 Deferred / 0.2.0):** Internet Archive search/IIIF UI,
`ia:` scheme, Hypertia multi-machine product, WWW HTML link graphs, rich AI
aux. Those do not change the core tables below.

Work plan: [PLAN.md](PLAN.md).

Related: [DESIGN.md](../DESIGN.md) (current shipping model), [TAGS.md](../TAGS.md),
[TILES.md](../TILES.md). Consumers: biltoo, dirtoo (via thumtoo library).

---

## 1. Goals and non-goals

### Goals

- Integer primary keys; digests only as indexes / wire form.
- **Blob** = dumb bytes; **media** = interpretation; **region** = place inside media.
- Archive members hashable; nested `//archive:` supported in the model.
- **Tiles only** for durable pixels (no multi-edge `levels` table).
- Directory **first open from DB cache** (slow FS/USB must not block list UI).
- Overlays (tags, sets, links, bookmarks) do not rewrite source files.
- dirtoo checksum/tag/set needs and biltoo image/document needs on one library.

### Non-goals

- Back-compat with current `content_id TEXT` ladder schema.
- Session edit identity (biltoo `SessionImageId`, live crop) — app-local.
- Full Hypertia/IA product in the first cut.

---

## 2. Three database roles

| Role | Suggested location (TBD exact names) | Contents |
|------|--------------------------------------|----------|
| **Index** | `$XDG_CACHE_HOME/thumtoo/index.sqlite` | Small meta: blob, hash, locator, members, media, region, directory snapshots, schema_meta, extracted structure metadata |
| **Bulk** | `$XDG_CACHE_HOME/thumtoo/bulk.sqlite` | Large BLOBs: tile payloads, HTTP body cache |
| **User** | `$XDG_STATE_HOME/thumtoo/user.sqlite` | Tags, tag defs, collections, bookmarks, user link edges, user annotations |

Rationale: cache wipe rebuilds index+bulk without destroying tags/bookmarks.
Bulk growth (tiles) must not bloat the hot index. User data may later be
synced or backed up independently.

All three share the same **logical** id space for `blob.id` only in the sense
that user rows reference digests or blob ids that exist when written; after a
cache wipe, user rows still key by **`blob:sha256:…` text** or re-resolve via
`blob_hash` when the blob row is recreated. Prefer storing **canonical text
refs** in user-data for survival across cache rebuilds; integer `blob_id` in
index/bulk only.

**Practical rule:**

- Index/bulk tables: integer FKs to `blob(id)`.
- User tables: prefer `blob_ref TEXT` (`blob:sha256:…`) so wipe + rehash still
  matches; optional cached `blob_id` if index is open.

---

## 3. URI conventions (canonical text)

Used in `locator.uri`, link ends, export, Hypertia later.

```text
blob:sha256:<64 lowercase hex>           -- preferred public blob id
file:///abs/path/file.pdf
file:///abs/path/a.zip//archive:member.jpg
file:///outer.zip//archive:inner.pdf//page:3
file:///book.pdf//page:12                -- pipe, not #page=
https://example.com/file.pdf
```

- **Pipes:** `//archive:…`, `//page:N` (1-based). Nested left-to-right.
- **Not** JAR `path!/member` for new writes.
- Fragments `#page=` may be accepted on input and rewritten to `//page:`.

---

## 4. Index database schema

SQLite, WAL. Types are SQLite affinities with intent noted in comments.

```sql
-- ---------------------------------------------------------------------------
-- schema_meta: key/value for epoch, feature flags, policy fingerprints
-- ---------------------------------------------------------------------------
CREATE TABLE schema_meta (
  key   TEXT PRIMARY KEY,
  value TEXT NOT NULL
);
-- schema_version: integer epoch for this redesign (start at 1 for new world)
-- tile_size: e.g. 256
-- ...

-- ---------------------------------------------------------------------------
-- blob: dumb byte identity (internal PK)
-- ---------------------------------------------------------------------------
-- Rationale: every durable FK inside index/bulk points here. Wire form is
-- always a digest from blob_hash, not this integer.
CREATE TABLE blob (
  id         INTEGER PRIMARY KEY,
  size       INTEGER,              -- byte length when known; NULL if unknown
  status     INTEGER NOT NULL DEFAULT 0,  -- 0=unknown, 1=ok, 2=missing, 3=error, ...
  created_at INTEGER NOT NULL,     -- unix seconds
  updated_at INTEGER NOT NULL
);

-- ---------------------------------------------------------------------------
-- blob_hash: digests (extensible algos without ALTER on blob)
-- ---------------------------------------------------------------------------
-- Rationale: dirtoo stores sha256/md5/sha1/crc32; tags require full sha256.
-- UNIQUE(algo, digest) enables lookup; UNIQUE(blob_id, algo) one digest per algo.
CREATE TABLE hash_algo (
  id   INTEGER PRIMARY KEY,
  name TEXT NOT NULL UNIQUE        -- 'sha256', 'sha1', 'md5', 'crc32', ...
);

CREATE TABLE blob_hash (
  blob_id INTEGER NOT NULL REFERENCES blob(id) ON DELETE CASCADE,
  algo_id INTEGER NOT NULL REFERENCES hash_algo(id),
  digest  BLOB NOT NULL,           -- raw bytes (32 for sha256), never hex TEXT
  PRIMARY KEY (blob_id, algo_id),
  UNIQUE (algo_id, digest)
);

CREATE INDEX idx_blob_hash_digest ON blob_hash(algo_id, digest);

-- ---------------------------------------------------------------------------
-- locator: where bytes were found (path, URL, nested archive chain as URI)
-- ---------------------------------------------------------------------------
-- Rationale: many locators → one blob after hash. size+mtime invalidate path
-- rows without re-reading whole trees.
CREATE TABLE locator (
  id         INTEGER PRIMARY KEY,
  uri        TEXT NOT NULL UNIQUE, -- canonical location string
  blob_id    INTEGER REFERENCES blob(id) ON DELETE SET NULL,
  size       INTEGER,              -- fingerprint of the *outer* file when applicable
  mtime_ns   INTEGER,              -- nanoseconds; NULL if unknown
  updated_at INTEGER NOT NULL
);

CREATE INDEX idx_locator_blob ON locator(blob_id);

-- ---------------------------------------------------------------------------
-- container_member: TOC of an archive blob + optional member blob identity
-- ---------------------------------------------------------------------------
-- Rationale: listing does not require hashing members. member blob_id set when
-- hashed (full read / explicit / tag / bookmark). Nested archives: member path
-- may itself be an archive blob with its own container_member rows.
CREATE TABLE container_member (
  container_id       INTEGER NOT NULL REFERENCES blob(id) ON DELETE CASCADE,
  member_path        TEXT NOT NULL,     -- normalized path inside archive
  is_directory       INTEGER NOT NULL DEFAULT 0,
  uncompressed_size  INTEGER,
  blob_id            INTEGER REFERENCES blob(id) ON DELETE SET NULL,
  PRIMARY KEY (container_id, member_path)
);

CREATE INDEX idx_container_member_blob ON container_member(blob_id);

-- ---------------------------------------------------------------------------
-- media: semantic interpretation of a blob
-- ---------------------------------------------------------------------------
-- Rationale: one table + kind, not separate image/video/audio tables.
-- kind: 1=image, 2=document, 3=video, 4=audio, 0=unknown, ...
CREATE TABLE media (
  id           INTEGER PRIMARY KEY,
  blob_id      INTEGER NOT NULL REFERENCES blob(id) ON DELETE CASCADE,
  kind         INTEGER NOT NULL DEFAULT 0,
  width        INTEGER,            -- display pixels after EXIF policy for images
  height       INTEGER,
  duration_ms  INTEGER,            -- video/audio
  page_count   INTEGER,            -- documents
  still_count  INTEGER,            -- planned video stills if used
  status       INTEGER NOT NULL DEFAULT 0,
  error_code   TEXT,
  updated_at   INTEGER NOT NULL,
  UNIQUE (blob_id, kind)           -- one row per kind interpretation; usually one
);

CREATE INDEX idx_media_blob ON media(blob_id);

-- ---------------------------------------------------------------------------
-- region: place inside a media object (page, full, …)
-- ---------------------------------------------------------------------------
-- Rationale: bookmarks, text layers, links, page tiles share region identity.
-- key is kind-specific stable string; ordinal optional for ordering.
-- kind: 0=full, 1=page, 2=archive_member_view, 3=epub_anchor, 4=time_range, ...
CREATE TABLE region (
  id         INTEGER PRIMARY KEY,
  media_id   INTEGER NOT NULL REFERENCES media(id) ON DELETE CASCADE,
  kind       INTEGER NOT NULL,
  key        TEXT NOT NULL,        -- e.g. '12' for page, CFI/spine for epub
  ordinal    INTEGER,              -- optional sort
  UNIQUE (media_id, kind, key)
);

CREATE INDEX idx_region_media ON region(media_id);

-- Optional alternate keys for the same region (EPUB multi-means addressing).
CREATE TABLE region_key (
  region_id INTEGER NOT NULL REFERENCES region(id) ON DELETE CASCADE,
  key_kind  INTEGER NOT NULL,      -- spine, cfi, ordinal, ...
  key       TEXT NOT NULL,
  PRIMARY KEY (region_id, key_kind),
  UNIQUE (region_id, key_kind, key)
);

-- ---------------------------------------------------------------------------
-- codec: small lookup for tile (and future) encodings
-- ---------------------------------------------------------------------------
CREATE TABLE codec (
  id   INTEGER PRIMARY KEY,
  name TEXT NOT NULL UNIQUE        -- 'jpeg', 'jxl', 'png', ...
);

-- ---------------------------------------------------------------------------
-- tile: pyramid metadata in index; payload in bulk DB
-- ---------------------------------------------------------------------------
-- Rationale: tiles-only pixel store. EVERY tile belongs to a region:
--   image  → one region(kind=full)
--   PDF    → region(kind=page, key=N); live or baked raster
-- PK includes region_id so pages cannot collide. No per-page media rows in v1.
-- scale/x/y = app pyramid (not format-native tile grids).
-- Completeness = query over tile rows for (media_id, region_id).
-- HTML fragment regions usually have zero tiles (identity only).
CREATE TABLE tile (
  media_id   INTEGER NOT NULL REFERENCES media(id) ON DELETE CASCADE,
  region_id  INTEGER NOT NULL REFERENCES region(id) ON DELETE CASCADE,
  scale      INTEGER NOT NULL,
  x          INTEGER NOT NULL,
  y          INTEGER NOT NULL,
  width      INTEGER NOT NULL,      -- pixel size of this tile
  height     INTEGER NOT NULL,
  codec_id   INTEGER NOT NULL REFERENCES codec(id),
  quality    INTEGER,
  PRIMARY KEY (media_id, region_id, scale, x, y)
);

-- ---------------------------------------------------------------------------
-- directory_snapshot + directory_entry: cache-first folder open
-- ---------------------------------------------------------------------------
-- Rationale: first paint from DB; inotify/watchers refresh later. dir_uri is
-- a locator.uri (file:///… directory form as defined by the library).
CREATE TABLE directory_snapshot (
  dir_uri    TEXT PRIMARY KEY,
  size       INTEGER,              -- optional aggregate fingerprint
  mtime_ns   INTEGER,
  listed_at  INTEGER NOT NULL,     -- when this snapshot was written
  incomplete INTEGER NOT NULL DEFAULT 0
);

CREATE TABLE directory_entry (
  dir_uri    TEXT NOT NULL REFERENCES directory_snapshot(dir_uri) ON DELETE CASCADE,
  name       TEXT NOT NULL,        -- single path component
  child_uri  TEXT,                 -- full locator uri for the child
  is_dir     INTEGER NOT NULL,
  size       INTEGER,
  mtime_ns   INTEGER,
  PRIMARY KEY (dir_uri, name)
);

-- ---------------------------------------------------------------------------
-- doc_structure: extracted text layers, links, outlines (cache, not user truth)
-- ---------------------------------------------------------------------------
-- Rationale: payload may be large but still "meta"; if oversized, move payload
-- to bulk and keep only hash/length here. layout_key namespaces renderer policy.
-- kind: 1=text_layer, 2=link, 3=image_map, 4=outline, ...
-- source: 1=extract (only in v1 for this table)
CREATE TABLE doc_structure (
  id          INTEGER PRIMARY KEY,
  media_id    INTEGER NOT NULL REFERENCES media(id) ON DELETE CASCADE,
  region_id   INTEGER REFERENCES region(id) ON DELETE CASCADE, -- NULL = doc-global
  layout_key  TEXT NOT NULL DEFAULT '',
  kind        INTEGER NOT NULL,
  source      INTEGER NOT NULL DEFAULT 1,
  payload     BLOB NOT NULL,
  updated_at  INTEGER NOT NULL
);

CREATE INDEX idx_doc_structure_media ON doc_structure(media_id, kind);
CREATE INDEX idx_doc_structure_region ON doc_structure(region_id);

-- ---------------------------------------------------------------------------
-- link_edge in index: optional *extracted* links only (source=extract)
-- User trails live in user.sqlite (same shape) so cache wipe keeps user links.
-- ---------------------------------------------------------------------------
-- Canonical text refs only (see §3). No polymorphic integer FK.
CREATE TABLE link_edge (
  id         INTEGER PRIMARY KEY,
  from_ref   TEXT NOT NULL,
  to_ref     TEXT NOT NULL,
  rel        TEXT,                 -- optional relationship label
  source     INTEGER NOT NULL,     -- 1=extract, 2=user (user DB), ...
  created_at INTEGER NOT NULL,
  UNIQUE (from_ref, to_ref, rel, source)
);

CREATE INDEX idx_link_from ON link_edge(from_ref);
CREATE INDEX idx_link_to ON link_edge(to_ref);
```

---

## 5. Bulk database schema

```sql
CREATE TABLE schema_meta (
  key   TEXT PRIMARY KEY,
  value TEXT NOT NULL
);

-- Tile payloads; coordinates match index.tile
CREATE TABLE tile_blob (
  media_id   INTEGER NOT NULL,    -- same ids as index (no cross-DB FK)
  region_id  INTEGER NOT NULL,
  scale      INTEGER NOT NULL,
  x          INTEGER NOT NULL,
  y          INTEGER NOT NULL,
  codec_id   INTEGER NOT NULL,
  data       BLOB NOT NULL,
  PRIMARY KEY (media_id, region_id, scale, x, y)
);

-- HTTP(s) body cache (locator or URL string)
CREATE TABLE http_body (
  url         TEXT PRIMARY KEY,
  fetched_at  INTEGER NOT NULL,
  blob_id     INTEGER,             -- set once hashed into index
  data        BLOB NOT NULL
);
```

Rationale: no cross-database foreign keys in SQLite; **logical** consistency is
maintained by the library (delete tile_blob when media goes away). `media_id`
values are allocated only in the index DB.

---

## 6. User database schema

Survives cache wipe. Keys by **canonical blob ref text** where possible.

```sql
CREATE TABLE schema_meta (
  key   TEXT PRIMARY KEY,
  value TEXT NOT NULL
);

-- Tag catalog (dirtoo TagStore parity)
CREATE TABLE tag_def (
  id         INTEGER PRIMARY KEY,
  name       TEXT NOT NULL UNIQUE,
  label      TEXT,
  color      TEXT,
  badge      TEXT,
  created_at INTEGER NOT NULL
);

-- Tags on content identity (sha256 wire form)
CREATE TABLE blob_tag (
  blob_ref   TEXT NOT NULL,        -- 'blob:sha256:' || hex
  tag_id     INTEGER NOT NULL REFERENCES tag_def(id) ON DELETE CASCADE,
  tagged_at  INTEGER NOT NULL,
  source     TEXT,                 -- 'user', 'auto', app id
  PRIMARY KEY (blob_ref, tag_id)
);

CREATE INDEX idx_blob_tag_tag ON blob_tag(tag_id);

-- Collections / sets (dirtoo FileSet direction; exclusivity TBD in app policy)
CREATE TABLE collection (
  id         INTEGER PRIMARY KEY,
  label      TEXT,
  color      TEXT,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL
);

CREATE TABLE collection_member (
  collection_id INTEGER NOT NULL REFERENCES collection(id) ON DELETE CASCADE,
  blob_ref      TEXT NOT NULL,
  ordinal       INTEGER,
  path_key      TEXT,              -- optional last-known path for display
  PRIMARY KEY (collection_id, blob_ref)
);

-- Bookmarks (reading position or favorite target)
CREATE TABLE bookmark (
  id         INTEGER PRIMARY KEY,
  target_ref TEXT NOT NULL,        -- blob ref, or blob ref + //page:N, etc.
  title      TEXT,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL
);

CREATE INDEX idx_bookmark_target ON bookmark(target_ref);

-- User link trails (same shape as extract links; separate DB)
CREATE TABLE link_edge (
  id         INTEGER PRIMARY KEY,
  from_ref   TEXT NOT NULL,
  to_ref     TEXT NOT NULL,
  rel        TEXT,
  source     INTEGER NOT NULL DEFAULT 2,  -- user
  created_at INTEGER NOT NULL,
  UNIQUE (from_ref, to_ref, rel, source)
);

CREATE INDEX idx_user_link_from ON link_edge(from_ref);
CREATE INDEX idx_user_link_to ON link_edge(to_ref);

-- User annotations (highlights, notes); geometry optional BLOB/JSON
CREATE TABLE annotation (
  id         INTEGER PRIMARY KEY,
  target_ref TEXT NOT NULL,        -- region or blob ref
  kind       INTEGER NOT NULL,     -- tag, highlight, note, ...
  body       TEXT,
  geom       BLOB,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL
);

CREATE INDEX idx_annotation_target ON annotation(target_ref);
```

---

## 7. Policies (normative for implementers)

### Hashing

- **File locator:** hash when needed for tags/trails/export or when fully read
  for decode; store `blob_hash` and set `locator.blob_id`.
- **Archive member:** not on TOC alone; when fully read, user requests checksum,
  or overlay requires blob identity — then fill `container_member.blob_id`.
- **Canonical public id:** SHA-256 when present (`blob:sha256:` + hex).

### Pixels

- Write **tiles** only into index+bulk.
- “Soft edge ≤512” for biltoo compatibility: **assemble from tiles** (or decode
  a transient raster); do not add a `levels` table.
- After biltoo 0.1.0: host renders from tiles directly.

### Directories

- `list_dir` UI path: read `directory_entry` for `dir_uri` if snapshot exists and
  policy accepts it; do not block first paint on full FS walk.
- Watchers update snapshots asynchronously.

### Provisional blobs

- Rows may exist with `blob` and `locator` but no `blob_hash` yet.
- Internal FKs use `blob.id`.
- Export / hypertia / durable external link: compute SHA-256 first, then emit
  `blob:sha256:…`.

### EXIF / display size

- Unchanged product rule: stored image `width`/`height` are **display** size
  after EXIF orientation policy (see existing EXIF tip); tiles are upright.

---

## 8. Mapping from current schema (conceptual)

| Today | New |
|-------|-----|
| `content.content_id` TEXT PK | `blob.id` + `blob_hash` |
| `locators` | `locator` |
| `archive_entries` | `container_member` (adds member `blob_id`) |
| `levels` / `level_blobs` | **dropped**; use `tile` / `tile_blob` |
| `tiles` / `tile_blobs` | `tile` + bulk `tile_blob` + `codec` |
| `tags` on content_id | user `blob_tag` on `blob_ref` |
| `document_index` | `media.page_count` (+ locator fingerprints) |
| `text_layers` / outlines | `doc_structure` |
| `directory_*` | same role, kept |
| `http_bodies` | bulk `http_body` |

---

## 9. Library API sketch (unchanged spirit)

```text
open(cache_root, user_root)
get_size / request_size (uri)
get_media / probe
list_container (archive uri) -> members
get_tile / request_tile
list_directory (dir_uri) -> from snapshot first
get_tags / add_tag (blob_ref or uri)
link_add / link_list / backlinks (canonical refs)
```

GUI: cache-only gets; workers for probe/hash/tile; one writer discipline per DB.

---

## 10. Implementation order

1. index: schema_meta, blob, hash_algo, blob_hash, locator  
2. container_member + //archive: resolution  
3. media + region (+ region_key)  
4. codec + tile + bulk tile_blob  
5. directory_snapshot / entry  
6. user: tag_def, blob_tag, collection*  
7. link_edge (user + extract), bookmark, annotation  
8. doc_structure  
9. http_body + HTTPS fetch  

---

## 11. Open items (narrow — do not block review of the rest)

1. SHA-256 as sole **merge** key when multiple digests disagree (policy).  
2. EPUB `region_key` key_kind values and text-overlay derivation.  
3. Collection membership exclusive vs multi (app policy vs UNIQUE).  
4. Final XDG file names; multi-process lock for user.sqlite.  
5. Directory snapshot TTL / stale policy without inotify.  
6. Whether `doc_structure.payload` lives in index or bulk by size threshold.

---

## 12. Deferred (0.2.0+ — not part of this schema review)

Track in project TODO, not as blockers for coding §4–6:

- Internet Archive **search** UI and **IIIF-first** book reading  
- **`ia:`** URI scheme vs only `https://archive.org/…`  
- WWW **HTML as link container** extraction  
- **Hypertia** multi-machine product (wire refs stay §3; HTTP shapes elsewhere)  
- media_aux (attention maps, embeddings) as extra user/index tables later  

---

## 13. Rationale summary

| Choice | Why |
|--------|-----|
| Integer blob PK | Avoid TEXT hash on every tile/tag row |
| blob_hash side table | Extensible algos; dirtoo multi-hash |
| media + kind | One probe API for biltoo/dirtoo |
| region + optional region_key | Pages + EPUB multi-address |
| tiles only | Single pixel source of truth |
| Three DBs | Wipe cache ≠ wipe tags; bulk ≠ hot index |
| Text link refs | Simple; needs canonicalize() |
| Directory cache-first | Cold USB/NFS must not block open |
| User blob_ref text | Tags survive index rebuild |
| thumtoo library | Shared lower layer for dirtoo and biltoo |

---

*End of schema-for-review. Comments welcome on table shapes, FK/ON DELETE
choices, and the index vs user split for `link_edge`.*
