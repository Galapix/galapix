<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# thumtoo design

> **Historical note (2026-09):** Sections that describe a schema-4 `content_id`
> TEXT ladder, soft pixel levels, and a dual-path `Database`/`BlobStore` are
> archival. The live implementation is the redesign **Store** (schema ≥ 100;
> see [docs/DATABASE.md](docs/DATABASE.md) and [docs/API_MIGRATION.md](docs/API_MIGRATION.md)).
> The C++ `Database` / `BlobStore` types were deleted in tip **272**.

## 1. Problem

Apps such as [biltoo](https://github.com/Grumbel/biltoo) currently keep image
sizes and soft previews in **process memory**. Opening a large session or an
archive repeatedly:

- re-probes dimensions (or falls back to neutral placeholders and wrong layout);
- re-decodes the same preview edges;
- re-lists archive members.

[dirtoo](https://github.com/Grumbel/dirtoo) already has pieces of the puzzle
(checksum SQLite, media meta cache, archive index, desktop thumbnailer client)
but no owned multi-edge **pixel ladder**. [galapix](https://github.com/Galapix/galapix)
implemented a full **tile pyramid** in SQLite, tightly coupled to that viewer.

**thumtoo** extracts a shared, library-shaped answer: durable index + display
pixels, consumable by biltoo first and optionally dirtoo later.

## 2. Lessons from sister projects

### galapix

- SQLite `files`: url, size, mtime, width, height.
- SQLite `tiles`: `(fileid, scale, x, y)` → JPEG/PNG blob.
- Archive members as URLs (`file://….rar//rar:inner.jpg`).
- Background DB thread; CLI `thumbgen` / `prepare` to prewarm.
- Strength: multi-resolution zoom. Weakness: app-coupled, many tile rows, blobs
  in DB.

### dirtoo

- **ChecksumStore**: path → digests; valid if size+mtime match.
- **MediaMetaCache**: async width/height (and media fields); GUI never probes on
  the UI thread.
- **dirtoo-archive**: read-only TOC + extract-on-demand (libarchive).
- **dirtoo-thumbnail**: D-Bus **Thumbnailer1 client**, not a pyramid owner.
- Strength: modular libraries, XDG paths, fingerprint invalidation.

### biltoo (consumer requirements)

- Replace provisional layout sizes with **known native size** as soon as probed
  once.
- Filmstrip / Gallery / Image soft preview from a **shared ladder**.
- Archive open without re-walking TOC every time.
- Keep **SessionImageId** for edits; thumtoo keys are **source URIs** only.

## 3. Goals

1. Permanent **native size** (and cheap format metadata).
2. **Archive TOC** cache + member identity.
3. **Display pixel ladder** at fixed long edges (e.g. 128, 256, 512, 1024, 2048).
4. **In-process C++ API** first; optional **D-Bus** service later for sharing and
   crash isolation.
5. GPL-3.0-or-later, REUSE headers, testable without a GUI.

## 3a. Hard product rules (normative)

### Ladder blob storage

Encoded display-ladder payloads live in **`blobs.sqlite`** under the cache
root (WAL, same process writer queue as the index). Metadata (`levels`
rows, sizes, locators) stays in **`index.sqlite`**. This avoids hundreds of
thousands of tiny files on large media trees while keeping the index
compact and queryable. Payload and index are separate files so vacuum and
backup policy can differ.

### Cache-first browse

The UI may browse **entirely from local cache** with **no I/O to source
volumes** until detail level or an explicit/idle refresh requires it.

Spinning-down USB/external HDDs and sleeping NAS shares commonly take
**10–30 seconds** to respond to the first I/O after idle. That latency must
not gate scrolling, folder open (when a snapshot exists), or painting known
previews — the user cannot even see what is in a folder to decide what to do
next if open blocks on spin-up.

- `get_*` / `list_cached_*` — local SQLite + blob files only.
- `request_*` / `prepare` / `refresh` — only place that touches sources.

Stale cache is acceptable; mark incomplete/unverified rather than block.

### Storage location

All durable state lives under **`$XDG_CACHE_HOME/thumtoo/`** (or an explicit
cache root for tests). **Never** write into the user’s directory trees:

- no sidecar files next to images
- no extended attributes on source files
- no `.DS_Store`, `Thumbs.db`, or other pollutants in source trees

The cache is **strictly read-only with respect to source data**.

### Content identity (hashes)

Couple durable media rows to **content** when possible, not only to path:

- Path + `(size, mtime)` remains a fast invalidation fingerprint.
- **Checksum (e.g. SHA-256)** is the stable id when directory layout changes
  (rename, reorganize, copy). Same bytes → same size/ladder/tags.
- Path indexes are convenience aliases that may point at a content id.

dirtoo’s checksum store and **tags-on-checksum** model are the reference;
thumtoo should align so apps can share identity.

### Tags (planned)

Optional **tags attached to content hash** (not path), same idea as dirtoo:

- Survive renames and moves.
- Library API for list/add/remove; UI stays in apps (dirtoo Tag Manager, etc.).
- Phase may trail the pixel ladder; schema includes optional `source` /
  `created_at` so we do not paint into a corner.
- **Reconcile with dirtoo’s checksum-tag schema before implementation** so the
  two stores can interoperate or share identity without a later migration.

### Network URLs (later)

**http(s)** source URIs: optional **libcurl** (`THUMTOO_HAVE_CURL`) — GET into
memory with size cap, used by `read_source_bytes` / size probe / tiles / pixels.
In-process **HTTP body cache** (session, 512 MiB) plus durable **`http_bodies`**
in `blobs.sqlite` (TTL `kHttpCacheTtlSeconds`, default 7 days). Lookup order:
RAM → disk → network.
**Plain local files first**, then archives, then remote URLs.

Library helpers (`thumtoo/uri.hpp`): `parse_location` / `format_location` understand
nested `//archive:` and `//page:` pipes, `is_http_uri` / `is_content_id_uri`, and
composers `with_archive_member` / `with_pdf_page`. Fetch of http(s) is not implemented yet. **Content-id resolve** (cache):
`meta_for_uri("sha256:…")`, `Client::resolve_content_id`, and
`list_uris_for_content_id` map between location URIs and durable identity.
`Client::read_source_bytes` loads original file or archive-member bytes
(source I/O, size-capped). Content-id tries each known locator. PDF pages
and http(s) are not covered yet.

## 4. Non-goals (initial)

- Full image editor or session/project file format.
- Replacing Freedesktop thumbnailers for every desktop icon.
- Replacing every desktop thumbnailer.
- Gigapixel region tiles (galapix-style) in phase 1.

## 5. Preview vs tiles vs original (pixel cache policy)

Durable display pixels are **not** a full multi-edge ladder by default.

| Layer | Durable? | Role |
|-------|----------|------|
| **Size / identity** | Yes | Layout, invalidation, content id |
| **Single preview** (JXL, one long-edge ≤ request) | Yes | Fast first paint / filmstrip (`request_pixels`) |
| **Grid tiles** 256² JPEG | Yes (optional) | Smooth zoom for Galapix (`request_tile`) |
| **Multi-edge ladder** (128+256+512+…) | No (on demand) | Derive by another `request_pixels` or downscale in the app |
| **Original full-res** | No | Viewer decodes past max cached tile scale |

`get_pixels` / `request_pixels(uri, max_edge)` remain the biltoo API: encode or
return the best **stored** level with `edge ≤ max_edge`. Encoding uses one
`vips_thumbnail` pass (JPEG shrink-on-load) for that edge only.

Galapix primary path: size → optional small preview → tiles; not a second
whole-image pyramid parallel to tiles.

## 6. Architecture

```
Apps: biltoo · dirtoo · thumtoo-prepare
                    │
                    ▼
         thumtoo::Client  (get / request + callback)
     ┌──────────┬──────────┬──────────┬──────────┐
     │ Identity │  Meta    │ Preview  │ Archive  │
     │ SourceId │  SQLite  │ + tiles  │ TOC+read │
     └──────────┴──────────┴──────────┴──────────┘
                    │
              worker pool + single SQLite writer
```

### Identity

Two layers:

1. **Locator URI** (how to find bytes today) — follow dirtoo / Galapix Location form:

   - File: `file:///absolute/path/to/image.jpg`
   - Archive member: `file:///absolute/path/to/book.zip//archive:member/path.jpg`
   - Nested archive: `file:///outer.zip//archive:inner.rar//archive:path/to/image.jpg`
   - Archive root (TOC only): `file:///absolute/path/to/book.zip//archive`
   - Later: `https://example.com/…` (not phase 1)

   The `//` acts as a pipe into the blob; `//archive:` (or historically `//rar:`, `//zip:`) selects the archive handler and optional member path. Prefer this over JAR-style `archive:…!…` (still accepted only for import compatibility).

2. **Content id** (what the bytes are): `sha256:<hex>` when known.

Path/mtime fingerprint of the **outer** file is a **fast** staleness check.
When the checksum is known, **levels, size, and tags** key primarily by content
id so renames do not orphan the ladder. Locators are many-to-one aliases onto
content rows.

Biltoo **SessionImageId** remains session/edit identity and must **not** key
durable pixels or tags.

### Schema (sketch)

```text
schema_meta (
  key TEXT PRIMARY KEY,          -- e.g. 'schema_version', 'ladder_edges', 'webp_quality'
  value TEXT
)
-- schema_version starts at 1; bump on incompatible SQLite or blob layout changes.
-- ladder_edges / webp_quality recorded so an old cache can be detected vs new policy.

content (
  content_id TEXT PRIMARY KEY,   -- see "Provisional ids" below
  width INTEGER,
  height INTEGER,
  format TEXT,
  duration_ms INTEGER,           -- NULL for pure images; set for video
  still_count INTEGER,           -- planned video stills N (e.g. 16); NULL for images
  status INTEGER NOT NULL,       -- enum: see Status below
  error_code TEXT,               -- optional machine code when status=failed/unsupported
  updated_at INTEGER
)

locators (
  uri TEXT PRIMARY KEY,          -- file:///… or file:///…//archive:member (later https:)
  content_id TEXT,               -- nullable until hashed
  outer_path TEXT,
  member_path TEXT,
  size INTEGER,
  mtime_ns INTEGER,              -- fingerprint of outer file
  updated_at INTEGER
)

archive_entries (
  archive_uri TEXT,
  member_path TEXT,
  uncompressed_size INTEGER,
  PRIMARY KEY (archive_uri, member_path)
)

-- Optional: directory listing snapshots (cache-first folder browse)
directory_snapshots (
  dir_uri TEXT PRIMARY KEY,
  size INTEGER,
  mtime_ns INTEGER,
  listed_at INTEGER,
  incomplete INTEGER
)

directory_entries (
  dir_uri TEXT,
  name TEXT,
  child_uri TEXT,
  is_dir INTEGER,
  size INTEGER,
  mtime_ns INTEGER,
  PRIMARY KEY (dir_uri, name)
)

levels (
  content_id TEXT,
  max_edge INTEGER,
  frame_idx INTEGER NOT NULL DEFAULT 0,  -- 0 = image / video poster; 1..N video stills
  pts_ms INTEGER,                       -- optional timestamp for video frames
  width INTEGER,
  height INTEGER,
  codec TEXT,             -- e.g. webp
  quality INTEGER,        -- encoder quality used; helps detect policy drift
  path TEXT,              -- relative under blob root (never next to sources)
  PRIMARY KEY (content_id, max_edge, frame_idx)
)

tags (
  content_id TEXT,
  tag TEXT,               -- align with dirtoo checksum-tags; no namespace yet
  source TEXT,            -- 'user' | 'auto' | app id; optional
  created_at INTEGER,
  PRIMARY KEY (content_id, tag)
)
-- Value-bearing or namespaced tags deferred; reconcile with dirtoo before coding.

```

Default locations (**only** under XDG cache, never in source trees):

- Index: `$XDG_CACHE_HOME/thumtoo/index.sqlite`
- Blobs: `$XDG_CACHE_HOME/thumtoo/blobs/…`

### API sketch

```text
open(cache_root)
get_size(uri) -> optional<Size>       // SQLite only (width/height)
get_meta(uri) -> optional<Meta>       // size + duration_ms + still_count + …
request_size(uri, callback)           // probe if missing
get_pixels(uri, max_edge, frame_idx=0) -> Image  // 0 = image/poster
request_pixels(uri, max_edge, cb, frame_idx=0)
list_frames(uri, max_edge) -> [{frame_idx, pts_ms, ready}]
request_frames(uri, max_edge, cb)                 // all stills for video
list_archive(archive_path) -> TOC
read_member(archive, member) -> bytes
invalidate(uri | outer_path)
prepare(paths, edges[])               // CLI / idle prewarm
```

**GUI thread** may only call non-blocking get/try and schedule request_*.  
Workers decode/encode; one writer queue for SQLite.


### Video (still frames + optional animated)

Videos share the same content-id + locator model. The `content` row carries
`duration_ms` and `still_count` (planned N, initially 16; NULL for pure images).
Ready frames live only in `levels`; clients do not need a separate frame table.
Display proxies:

1. **Poster** — `frame_idx = 0` in `levels` (representative still, same as an
   image ladder entry).
2. **Temporal stills** — `frame_idx = 1 .. N` at the same `max_edge` values.
   Apps build contact sheets / scrubbers themselves; **thumtoo does not store
   a pre-tiled storyboard**.
3. **Animated preview** (must-have, deferred) — short muted low-res WebM/MP4;
   neither biltoo nor dirtoo consume it yet.

#### Count policy (initial)

Start simple and fixed:

```text
N = 16   // evenly spaced stills (plus poster as frame_idx 0)
```

`content.still_count` records the planned N so clients know how many frames
exist (or will exist) without scanning `levels`. Actual readiness is still
queried from `levels` / `list_frames`. Adaptive density (duration-based clamp
8–64) can be added later without schema changes—only the generation policy
and the value written to `still_count` change. Prefer keyframes when the
container exposes them.

#### Addressing frames — no special public URLs

Frames are **not** given their own locator URIs. The video’s content_id owns
all stills. Clients address them via the levels primary key
`(content_id, max_edge, frame_idx)` or the API:

```text
get_pixels(uri, max_edge, frame_idx=0) -> Image   // 0 = poster / plain image
list_frames(uri, max_edge) -> vector of {frame_idx, pts_ms, Image?}
request_frames(uri, max_edge, cb)                 // generate missing stills
```

A future optional Location form such as `file:///video.mp4//frame:3` is
possible for convenience but is **not** required and must not become a second
identity; it would only be a view onto the same content_id + frame_idx.

#### Worker isolation

Default path is **subprocess** (ffmpeg CLI or tiny helper) so broken files can
be killed without taking down the library or host app. In-process libav remains
an optional fast path once timeouts/sandboxing exist.

### D-Bus (phase 3)

Same operations on a session service (e.g. `local.Thumtoo1`). Client library
selects backend via config/env (`THUMTOO_MODE=local|dbus`). Not required for
biltoo MVP.


### Status enum (normative)

`content.status` is not a free integer. Values:

| Value | Name | Meaning |
|-------|------|---------|
| 0 | `pending` | Known locator; not yet successfully probed |
| 1 | `ready` | Size (and any requested levels) valid |
| 2 | `failed` | Probe/decode failed; may retry later (I/O, transient) |
| 3 | `unsupported` | Recognized as media but codec/container not handled |
| 4 | `incomplete` | Partial success (e.g. size known, some levels missing) |

UI must distinguish these: show placeholder for `pending`/`incomplete`, allow
retry for `failed`, and stop retrying `unsupported`. Never block the GUI waiting
for a transition out of `pending`.

### Provisional content ids

Until a content hash is known, a **provisional id** is allowed:

- Format: `prov:<uuid-v4>` (stable for the life of the row).
- Created when the first locator is inserted without a hash.
- **Promotion**: when SHA-256 of the bytes is computed, either
  1. update `content_id` in place if no conflicting `sha256:…` row exists, or
  2. merge into the existing `sha256:…` row and rewrite `locators`, `levels`,
     and `tags` foreign keys, then delete the provisional row.
- Levels/tags always follow the content row; promotion must be atomic on the
  single writer queue so no duplicate ladders remain.

### Directory snapshots (justified)

`directory_snapshots` / `directory_entries` stay in thumtoo, not dirtoo.

Spinning-down USB/external HDDs and sleeping NAS shares commonly take
**10–30 s** to respond to the first I/O after idle. A folder-open or scroll
must never block on that latency when a snapshot exists — this is a harder
real-time constraint than pixel/preview latency and is the primary reason
directory snapshots are owned here rather than left to live listing alone.

This is the same job as the pixel ladder (durable index of something
expensive to re-discover so the app never blocks on source hardware), applied
to directory contents instead of pixels.

- Snapshots are keyed by dir locator + outer size/mtime fingerprint; marked
  `incomplete` when listing was truncated or unverified.
- **Boundary:** dirtoo owns live listing, watching, and mutation
  (rename/move/delete) and decides *when* to write or refresh a snapshot;
  thumtoo owns the durable last-known-good snapshot consulted before the live
  listing resolves (or when source I/O is pending/unavailable).
- Visible in the README scope table so the split survives later boundary
  debates without relying on oral history of the 30-second number.

### Archive security (normative)

Alongside “never write into source trees”:

- **Member path sanitization**: reject or strip `..`, absolute paths, and
  nul bytes in archive member names before storing TOC or extracting.
- **Decompression limits** (schema_version 1 defaults; tunable):
  - max uncompressed size per member: **512 MiB**
  - max compression ratio (uncompressed/compressed): **100:1**
  - max total uncompressed bytes per single `prepare` of one archive: **2 GiB**
  Zip-bomb class inputs must fail closed (`status=failed` / `unsupported`),
  not fill the disk.
- Extraction is always into the cache blob area or a private temp dir under
  the cache root — never into the source tree.

### Concurrency and SQLite

- **WAL mode** is required for the index DB so a GUI process and
  `thumtoo-prepare` can coexist.
- **Single writer queue** inside each process; cross-process writers rely on
  SQLite locking + short transactions.
- **Priority**: interactive `request_*` (visible UI) preempts background
  `prepare` / idle prewarm. Implementation: two queues or a priority field on
  work items.
- **Backpressure**: unbounded `request_pixels` fan-out (e.g. opening a 10k
  folder) must not spawn unbounded workers. Cap concurrent decodes; coalesce
  duplicate (content_id, max_edge, frame_idx) work; excess requests wait or
  return “scheduled”.
- Callbacks **never** run on an arbitrary worker thread without a documented
  marshal path (see API contract).

### API threading contract

For biltoo (Qt) and other GUI hosts:

- `get_*` / `list_cached_*` / `get_meta` — **synchronous, non-blocking**,
  cache-only; safe on the GUI thread.
- `request_*` / `prepare` / `refresh` — schedule work; return immediately.
- Completion callbacks run on a **caller-supplied executor** (e.g. Qt event
  loop via `QMetaObject::invokeMethod`, or a user `post(fn)` hook registered
  at `open()`). The library does **not** assume a particular GUI toolkit and
  does **not** call application code from worker threads by default.
- Optional `std::future`-style wrappers may exist for CLI/tools; GUI path
  stays callback + executor.

### Cache lifecycle (named gap)

MVP may omit automatic eviction, but the design acknowledges unbounded growth:

- Ladder edges × images + 16 video stills per video will accumulate.
- Planned (Later): size or age cap, LRU or “least recently `get_*`” eviction,
  orphan sweep (locators whose outer path/mtime no longer match and no other
  locator remains), and `thumtoo-status` / `thumtoo-gc` CLI.
- Until then: document that the cache is append-mostly; operators may delete
  `$XDG_CACHE_HOME/thumtoo/` safely (regenerable).


## 6b. Phase 0 constants (schema_version = 1)

These are the implementable defaults; change only with a schema_version bump
or an explicit cache wipe.

| Constant | Value |
|----------|--------|
| `schema_version` | `1` |
| Ladder long edges | `128, 256, 512, 1024, 2048` |
| Default level codec | JPEG-XL (libvips) |
| Default JXL quality | `80` |
| Video still count | `16` (+ poster as `frame_idx` 0) |
| Content hash | SHA-256, id form `sha256:<hex>` |
| Provisional id | `prov:<uuid-v4>` |
| Archive max member uncompressed | 512 MiB |
| Archive max compression ratio | 100:1 |
| Archive max total extract / prepare | 2 GiB |
| SQLite journal | WAL |

`schema_meta` must record at least `schema_version`, `ladder_edges`, and
`jxl_quality` so a newer binary can detect an older policy and decide
regenerate vs serve-as-is.

## 7. Phases

| Phase | Deliverable |
|-------|-------------|
| **0** | This design; URI + fingerprint rules; spike size-only SQLite wired conceptually to biltoo |
| **1** | `libthumtoo`: sources + levels, async probe/decode, `thumtoo-prepare` CLI |
| **2** | Archive TOC + member pipeline; biltoo archive open uses cache |
| **3** | Optional `thumtood` + D-Bus |
| **4** | Optional grid tiles / deep zoom ([TILES.md](TILES.md)) |

## 8. biltoo integration (target)

| Pain | thumtoo |
|------|---------|
| Provisional 1000×1000 layout | Durable `get_size` after first probe |
| Gallery / filmstrip soft tiles | `get_pixels(uri, 256\|512)` — durable soft max **512** (`kMaxSoftLadderEdge`) |
| Gallery zoomed / Image native | consumer full decode or `request_tile` — **not** a larger soft ladder |
| Image-mode soft preview | `get_pixels(uri, 512\|1024)` |
| Archive re-list | Cached TOC |
| Slideshow warm | `prepare(session_paths, {512,1024})` |

### Ladder level provenance (`PixelSource`)

Each soft ladder level records how the pixels were produced so hosts never
confuse a tiny EXIF stand-in with a real shrink from the full source:

| `PixelSource` | Meaning |
|---------------|---------|
| `Unknown` (0) | Legacy cache row (pre-tagging) |
| `JpegShrink` (1) | `vips_thumbnail` / shrink-on-decode from full source |
| `Embedded` (2) | EXIF or other embedded JPEG thumbnail (may be small / off-colour) |
| `Full` (3) | Near-native extract stored as a level (e.g. `//pdfimage`) |

Exposed on `PixelLevel::source` from `get_pixels` / `request_pixels`. Prefer
`JpegShrink` or `Full` when choosing a display underlay; treat `Embedded` as a
fast first paint only until a proper soft level exists.



## 9. What not to do

- Store multi-megapixel full frames in SQLite BLOBs by default.
- Key durable pixels or tags by path alone when a content hash is known.
- Key durable pixels by biltoo session edit id.
- Block the GUI on archive listing, network readdir, or encode.
- Touch source trees (xattrs, sidecars, AppleDouble, …).
- Extract archive members without path sanitization or size/ratio caps.
- Call GUI/app callbacks directly from worker threads.
- Require http(s) for MVP.
- Vendor galapix/dirtoo sources into biltoo; keep thumtoo as its own repo.

## 10. License

GPL-3.0-or-later, REUSE.


## Format classification (single source)

Extension and MIME tables live in [`include/thumtoo/format.hpp`](include/thumtoo/format.hpp)
(`src/format.cpp`). Archive/PDF helpers and `prepare_paths` use that module.

Apps (Galapix, biltoo, `.desktop` files) should prefer:

* `classify_path` / `is_image_path` / `is_archive_path` / `is_pdf_path`
* `media_mime_types()` or `desktop_mime_types_line()` for file dialogs / MimeType=

Do not maintain a parallel extension list in each app.


## PDF layout DPI (2026-09-07)

Page `get_size` and live `request_tile` use **kPdfLayoutDpi (144)** so
interactive tiles are sharper than 72 dpi media-box. Rasterize on demand from
MuPDF; durable tile cache still fills on first request.


## Locator query (2026-09-07)

Cache-only: `list_locators_by_uri_prefix`, `list_locators_by_outer_path_prefix`,
`list_locators_like` (+ Client wrappers). Foundation for app-side library browse
without Galapix resource SQL.

## Inline LQIP (ThumbHash)

For gallery / list density, each blob may store a **ThumbHash/Handsum** LQIP
(~25–147 bytes). **Never generated standalone** — only opportunistically when
tile/soft encode already holds a free small raster. See
[docs/PIXEL_AND_ARCHIVE_POLICY.md](docs/PIXEL_AND_ARCHIVE_POLICY.md) §1.1.
Size probe is cache-only for LQIP (no source open).

Stack for progressive paint:

1. **LQIP** (content row) — instant soft color/structure
2. **Levels** (`request_pixels` / JXL ladder in blobs) — readable soft preview
3. **Grid tiles** (`request_tile`) — zoom/pan detail

API: `Client::get_lqip(uri)` (cache-only). See Galapix `ImageOverview` and
`GLOSSARY.md` (LQIP section).

