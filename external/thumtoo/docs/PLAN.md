<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Final plan — database redesign (pre-implementation)

Status: **approved for implementation** of the 0.1.0 spine.  
Normative schema detail: [DATABASE.md](DATABASE.md).  
This document is the work plan, locked decisions, API impact, and explicit
non-goals. Do not reopen large design threads unless implementation hits a
blocker listed under §7.

---

## 1. Objective

Replace the current TEXT `content_id` / dual ladder+tiles cache with:

- integer **blob** identity + **blob_hash**,
- **media** + **region** + **tiles-only** pixels,
- **container_member** for archives,
- **directory** snapshots for cache-first folder open,
- **user** store for tags / sets / bookmarks / link edges,

in a **thumtoo library** shared by biltoo and dirtoo.

**No migration** of old SQLite rows. On epoch detect: warn, replace cache DBs;
**never** wipe the user DB by default.

---

## 2. Locked decisions

| Topic | Decision |
|-------|----------|
| Blob PK | Integer `blob.id`; wire form `blob:sha256:<hex>` |
| Digests | Table `blob_hash` (algo + raw BLOB), not wide columns on `blob` |
| Media | Single `media` table + `kind` (image/document/video/audio/…) |
| Region vs archive | **Separate**: `container_member` = byte TOC; `region` = media view |
| URI children | Same *feel*: pipes `//archive:`, `//page:`; optional `list_children` API |
| Page tiles | PK **`(media_id, region_id, scale, x, y)`**; images always have `region(kind=full)` |
| Levels | **No** durable multi-edge levels; reconstruct soft views from tiles |
| biltoo pixels | Compat: library may synthesize ≤edge from tiles until host goes tile-native after 0.1.0 |
| Member hash | On full read, explicit request, or overlay need — not on TOC alone |
| Link edges | Canonical **text** refs + indexes; no ref_node for now |
| Provisional | Internal PK links; export/public ref forces hash |
| Directories | **First list from DB snapshot**; watchers refresh later |
| DB split | **Index** (meta) · **Bulk** (tiles, HTTP bodies) · **User** (tags, sets, bookmarks, user links) |
| Ownership | thumtoo = lower-layer library |
| PDF pages | Regions + renderer-backed tiles (live PDF allowed); not fake zip members |
| HTML fragments (later) | Regions (`//fragment:id`); tiles optional/often absent |
| Merge digest | Prefer **SHA-256** as identity when present; other algos are aliases |

---

## 3. Schema (summary)

Full SQL and comments: [DATABASE.md](DATABASE.md). Implementers must update
that file if the tile PK or three-DB split changes.

**Index:** `blob`, `hash_algo`, `blob_hash`, `locator`, `container_member`,
`media`, `region`, `region_key`, `codec`, `tile` (with **region_id NOT NULL**),
`directory_snapshot`, `directory_entry`, `doc_structure`, extract `link_edge`.

**Bulk:** `tile_blob` (same composite key as `tile`), `http_body`.

**User:** `tag_def`, `blob_tag` (on `blob_ref` text), `collection` /
`collection_member`, `bookmark`, user `link_edge`, `annotation`.

**Tile key (normative):**

```text
PRIMARY KEY (media_id, region_id, scale, x, y)
```

Every image has a `full` region. Document pages are `region(kind=page)`. No
per-page `media` rows required for tiling in v1.

---

## 4. API impact (thumtoo `Client`)

### Keep

- URI-first `get_*` (cache-only) / `request_*` (Executor callbacks).
- Threading contract (GUI get, worker request).
- Archive TOC refresh, document page count, basic tag get/add/remove (storage moves).

### Change

| Area | Change |
|------|--------|
| `open` | Index + bulk (+ user) roots; schema epoch check |
| Pixels | Tile-primary; `get_pixels(uri, max_edge)` optional **compat** via tile assembly |
| Identity | Resolve to blob; public `blob:sha256:`; drop durable TEXT content_id PK |
| Purge / invalidate | Touch index+bulk; user data only on explicit user-purge APIs |

### Add

| API | Role |
|-----|------|
| `list_directory` / `request_refresh_directory` | Cache-first dir open |
| `list_children` (optional sugar) | Members \| pages \| dir entries |
| Blob ensure / hash | Explicit checksum; member hash policy |
| Collections / links / bookmarks | User DB (can land after pixel spine) |

### Remove / deprecate

- Durable **level** ladder as source of truth.
- Preferring JAR `!/` URIs on write (parse legacy only if needed).

Feature macros: new schema epoch; `TILES_ONLY`; user-store present.

---

## 5. Implementation order

### Phase A — skeleton

1. Schema epoch constant; open three DBs; seed `hash_algo` / `codec`.  
2. Old cache detection → warn → recreate index+bulk (leave user).  
3. `blob` / `blob_hash` / `locator` + URI parse (`//archive:`, `//page:`).

**Status:** A done (`Store` open, blob/hash/locator).  
**Phase B status:** media + region + tiles on `Store`.  
**Phase C status:** `container_member` TOC + optional member blob_id (hash on
read/tag/explicit via `set_container_member_blob`); `ensure_document_media` /
`ensure_page_region`.  
**Phase D status:** complete (directory, user overlays, http_body, tile list).  
**Phase E (library dual-path → Store-only):** complete (≥257 default; ≥262
Client never opens legacy; ≥263 handlers Store-only only; ≥265 dead soft-level
env removed). Host cutover: [HOST_CUTOVER.md](HOST_CUTOVER.md).
See [API_MIGRATION.md](API_MIGRATION.md).
**Phase E complete (≥275):** Database/BlobStore deleted; tools Store-only;
forget-uri / uri-prefix / orphan GC; layout helpers cleaned. DESIGN ladder
sections are archival.

**Next:** host tile-native polish (biltoo PreferCache / filmstrip); richer
directory GC. Durable LQIP on Store: tip **277** (`blob_lqip`).

### Phase B — images

4. `media` + `region(full)` for still images.  
5. `tile` + bulk `tile_blob`; encode/decode path.  
6. Compat `get_pixels`/`request_pixels` assembling from tiles (if needed for biltoo).

### Phase C — archives & documents

7. `container_member` + TOC; hash policy on full read/tag.  
8. Document `media` + `region(page)` + page tiles (PDF path first).  
9. `document_page_count` / text layer via `doc_structure` as time allows.

### Phase D — directory & user

10. Directory snapshot read path (cache-first).  
11. User DB: tags (dirtoo-aligned), then collections, bookmarks, link_edge.  
12. Minimal HTTPS → `http_body` → blob (optional in same milestone if thin).

### Phase E — hosts

13. Point biltoo at new client (compat pixels).  
14. dirtoo: optional switch checksum/tags to library (can trail biltoo).  
15. After biltoo **0.1.0**: host tile-native rendering; shrink compat ladder API.

Each phase: small commits, tests for resolve + one happy path, bundle handoff
as usual.

---

## 6. Explicit non-goals (this effort)

- Migrating old ladder rows.  
- Internet Archive search / IIIF-first UI / `ia:` scheme (**0.2.0+**).  
- Hypertia multi-machine product (**sketch only** in older notes).  
- WWW HTML-as-graph extraction (**0.2.0+**); HTML fragment regions only if a
  concrete host need appears.  
- Full EPUB multi-key polish (stub `region_key` OK).  
- media_aux (attention, embeddings).  
- Dual process writers without a locking story (document single-writer or
  coarse lock for user.sqlite).

---

## 7. Stop-and-discuss triggers

Pause and revisit design only if:

- Tile PK cannot express a required pyramid without a third identity axis.  
- User-data vs cache split cannot satisfy dirtoo path+checksum+tag flows.  
- Directory cache-first violates a hard correctness requirement we cannot fix
  with “stale + refresh” semantics.  
- biltoo cannot ship 0.1.0 with tile assembly compat (performance).

Otherwise implement through Phase E.

---

## 8. Success criteria (0.1.0 library spine)

- [x] New empty DBs open; old epoch refused or wiped with clear message.  
- [x] Local image: locator → blob/hash → media → full region → tiles (Store-only default; legacy removed ≥262).  
- [x] Soft pixel fetch works for biltoo via tiles (compat TileSynth + Store).  
- [x] Zip member open via `//archive:`; member hash after probe; Store container TOC dual-write.  
- [x] PDF page via `//page:N`; tiles keyed by page region (dual-write).  
- [x] Directory list returns snapshot without mandatory FS walk on first paint (Client → Store).  
- [x] Tags on `blob:sha256:` survive index/bulk delete (user.sqlite; Store-only tags).  
- [x] No `levels` table in the new schema (Store epoch ≥ 100 is tiles-only; Client no longer writes levels).

---

## 9. Doc maintenance

- **DATABASE.md** — schema + rationale (update tile section to match §2).  
- **PLAN.md** — this file; mark phases done in TODO.md as work lands.  
- **BLOB_AND_OVERLAY.md** — pointer only.  
- API migration notes can be added as `docs/API_MIGRATION.md` when Client
  headers change.

---

*End of final plan. Library Client is Store-only (≥262). Remaining work is
host tile-native polish and optional dead-code cleanup in Client.*
