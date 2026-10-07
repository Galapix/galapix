<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Content appearance state (flip / rotate / crop)

## Intent

Remember non-destructive **content** appearance (flip, quarter-turns, optional
crop and colour grade) across app restarts **without** touching source files
or their directories.

This is **not** a biltoo project (`.biltoo`). Projects remain the portable
session + Workspace document. This store is a **local preference** keyed by
content identity so reopening the same bytes restores orientation.

## Layering (CONTENT-VARIANT)

| Layer | Owner | Key |
| ----- | ----- | --- |
| Content bytes | disk / archive / page | `content_id` = `sha256:<hex>` or `sha256:<hex>:page:<n>` |
| Session variant | biltoo | `SessionImageId` |
| Local appearance state | thumtoo state DB | `content_id` |

- Source files and directories are **never** modified.
- Durable ladder / tags stay under `XDG_CACHE_HOME` (cache).
- Appearance state and **`user.sqlite`** (tags/collections) live under **`XDG_STATE_HOME/thumtoo`** (user state, not cache).
- `SessionImageId` must **not** key this store (session-local; not durable).
- Same content opened twice in one session still gets **two** session ids;
  both may *seed* from this store, then diverge independently.

## Location

```text
${XDG_STATE_HOME:-$HOME/.local/state}/thumtoo/
  appearance.sqlite3
```

Fallback if neither `XDG_STATE_HOME` nor `HOME` is set: `/tmp/thumtoo-state/`.

Wipe policy: deleting the state directory forgets orientations; it does **not**
invalidate the pixel cache.

## Database

Single SQLite file, WAL mode, schema version 1.

```sql
CREATE TABLE schema_meta (
  key   TEXT PRIMARY KEY,
  value TEXT NOT NULL
);

CREATE TABLE content_appearance (
  content_id            TEXT PRIMARY KEY,  -- "sha256:" + lowercase hex
  version               INTEGER NOT NULL DEFAULT 1,
  updated_unix          INTEGER NOT NULL,
  content_h_flip        INTEGER NOT NULL DEFAULT 0,
  content_v_flip        INTEGER NOT NULL DEFAULT 0,
  content_quarter_turns INTEGER NOT NULL DEFAULT 0,
  has_crop              INTEGER NOT NULL DEFAULT 0,
  crop_x                INTEGER,
  crop_y                INTEGER,
  crop_w                INTEGER,
  crop_h                INTEGER,
  crop_source_w         INTEGER,
  crop_source_h         INTEGER,
  crop_rotation         REAL NOT NULL DEFAULT 0,
  -- Optional grade (0 = identity / unset)
  grade_brightness      INTEGER,
  grade_contrast        INTEGER,
  grade_saturation      INTEGER,
  grade_hue             INTEGER,
  grade_gamma           INTEGER
);
```

Identity rows (all flips false, turns 0, no crop, no grade) are **deleted**
rather than stored — keeps the DB sparse.

## Public API (`thumtoo/appearance.hpp`)

```text
default_state_root() -> path
AppearanceStore::open(state_root = default)
get(content_id) -> optional<ContentAppearance>
put(content_id, ContentAppearance)
remove(content_id)
```

`content_id` should be normalized (`sha256:` + lowercase hex). Helpers accept
bare hex and add the prefix.

Hashing source bytes uses existing `sha256_file_hex` / locator resolution;
callers that only have a path hash the file (or the archive member policy
defined by the consumer).

## biltoo integration

1. **Write** after a committed content edit (flip / rotate / crop apply) when
   the session image has a resolvable path → content_id.
2. **Read** when binding a new session image that has **no** entry in
   `SessionAppearanceStore` yet (and is not loading from a `.biltoo` row that
   already carries appearance). Seed `m_appearance` from the store, then bake
   through the normal install gate.
3. Project load / explicit session appearance **wins** over the state store.
4. Workspace pose is **not** stored here (session / project only).

## Out of scope (later)

- Per-variant durable ids (hash+uuid) for multi-edit of the same file
- GC of rows whose content_id no longer appears in any locator
- Sync / export of the state DB
- Writing appearance into sidecar files next to media (explicitly rejected)

## Format versioning

`version` column and `schema_meta.schema_version` allow additive columns.
Unknown grade fields remain nullable; readers treat NULL as identity.
