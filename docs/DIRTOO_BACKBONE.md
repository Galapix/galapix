<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# thumtoo as backbone for dirtoo — virtual cached filesystem

Status: **design** (2026-09-17). Not yet implemented beyond the existing
minimal `directory_snapshot` / `directory_entry` tables and
`Client::refresh_directory_snapshot`.

Related: [DATABASE.md](DATABASE.md), [DESIGN.md](../DESIGN.md),
[TAGS.md](../TAGS.md), dirtoo `ARCHITECTURE.md`.

---

## 1. Goal

dirtoo should treat the real filesystem as an **eventually consistent source of
truth**, not as the latency-critical path for every open, list, or attribute
read. First paint of a folder (local SSD, USB, NFS, or a previously visited
archive) must come from durable cache. Background work brings the cache into
agreement with the live tree.

**Architectural implication:** thumtoo becomes the shared, durable,
**read-only** filesystem metadata + content-identity layer that dirtoo (and
biltoo / galapix) consume. Mutations stay in dirtoo’s `dirops`. thumtoo never
writes to source trees.

Default product rule for overlays (tags, sets, bookmarks):

- **Everything lives in the user database.**
- **Nothing is written next to user files** (no sidecars by default).
- Optional per-directory external files (`.DS_Store`-style) are a **later**
  feature upgrade, not the default path.

---

## 2. What exists today

### 2.1 thumtoo

| Concern | Status |
|---------|--------|
| Content identity (`blob` + multi-algo hashes) | Implemented |
| Locators (`file://`, `//archive:`, `//page:`, …) | Implemented |
| Archive TOC (`container_member`) | Implemented |
| Media meta, tiles | Implemented |
| Directory snapshots | **Minimal** — name, child_uri, is_dir, size, mtime_ns |
| Tags / collections (user DB) | Implemented |
| Full POSIX metadata on directory entries | Absent |
| Background FS sync / watch patch API | Absent |
| Virtual POSIX-like read API for hosts | Absent |
| Tags/sets JSON export/import | Absent |

### 2.2 dirtoo (consumer side)

- Live `list_directory` / `FileInfo` (size, mtime, atime/ctime/birth where
  available, mode bits, dir/reg/symlink).
- Separate `ChecksumStore`, `MediaMetaCache`, archive TOC, `DirectoryWatcher`.
- No durable directory snapshot on the GUI path yet.
- Tags/sets keyed by content hash — same identity model as thumtoo user DB.

Overlap (checksums, media meta, archive TOC, tags) should converge on thumtoo
so fingerprints and “is this fresh?” answers do not diverge.

---

## 3. Schema and on-disk compatibility policy

### 3.1 Supersedes “cache is disposable”

The redesign-era note that hosts should treat the Store cache as disposable
across upgrades was appropriate **before the first release**, when epochs were
experimental and ladder→tile conversion was intentionally non-migrated.

**After release, that is no longer the default stance.**

| Store | Policy |
|-------|--------|
| **index.sqlite** / **bulk.sqlite** | Prefer **in-place migration**. Wipe or force-delete only when the layout is truly unrecoverable or the cost of migration exceeds value (document why). |
| **user.sqlite** | **Must** survive. Tags, sets, bookmarks, user links are user data. Never “delete and retry” as the primary upgrade path. |

API surface may still break between library versions; **SQLite contents** are
what must be protected.

### 3.2 Migration mechanics (index / bulk)

Current open path rejects `schema_version < kStore*SchemaVersion` with
“delete … and retry”. That is acceptable only as a **last resort**.

Going forward:

1. **Additive first.** New columns on `directory_entry` / `directory_snapshot`
   (and similar) via `ALTER TABLE … ADD COLUMN` with NULL-safe readers.
2. **Versioned steps.** `schema_meta.schema_version` advances one step at a
   time; each step has an explicit `migrate_index_vN_to_vN+1` (and bulk/user
   counterparts when needed).
3. **Feature flags optional.** Sub-keys in `schema_meta` (e.g.
   `directory_stat_rich=1`) let hosts detect capability without treating old
   rows as corrupt.
4. **Legacy rows stay valid.** Missing new fields mean “unknown”, not
   “invalid snapshot”. Refresh backfills lazily.
5. **Epoch wipe** only when: irreversible layout change, known corruption, or
   human-initiated reset. Log clearly; offer export of user data first when
   user DB is involved.

### 3.3 What is *not* a compatibility requirement

- Stable C++ API / POD field order across every tip (hosts recompile).
- Migrating pre-Store ladder `content_id TEXT` caches (already cut over).
- Preserving every experimental intermediate from the redesign branch.

---

## 4. Directory cache expansion (POSIX-like observation)

### 4.1 Intent

Cache-first `readdir` + `lstat`-class data so dirtoo can fill models without
blocking on the real FS when a valid snapshot exists. Special files, symlinks,
and optional xattrs are part of a **faithful observation**, not a media-only
subset.

### 4.2 Target fields (directory_entry and related)

Beyond today’s `name`, `child_uri`, `is_dir`, `size`, `mtime_ns`:

| Field | Notes |
|-------|--------|
| file_type | reg / dir / lnk / chr / blk / fifo / sock |
| mode | full `st_mode` or type + permission bits |
| uid, gid | owner/group |
| atime_ns, ctime_ns, birth_ns | where platform provides |
| nlink, inode, dev, rdev | hardlink awareness, specials |
| symlink_target | `readlink` result |
| flags | incomplete, inaccessible, … |
| blob_id | optional content identity when hashed |

Directory-level snapshot: strengthen fingerprint (mtime, optional inode/dev,
entry count); keep `listed_at`, `incomplete`, optional error.

**xattrs:** allowlist + size policy in v1; large values in bulk or side table.
Not required for a valid listing snapshot.

### 4.3 Host API shape (spirit)

- `get_*` — cache only, GUI-safe (warm listing hits must not enter the worker
  queue; same lesson as tile cache-hit-fast).
- `request_refresh_directory` — worker + Executor.
- `apply_watch_delta` — incremental patch from dirtoo’s `DirectoryWatcher`.
- Full `replace_directory_snapshot` remains the correct fallback.

### 4.4 Non-goals for thumtoo

- Mutations (mkdir, rename, chmod, setxattr).
- Guaranteeing live consistency without a watcher.
- FUSE mount in the first delivery (optional later).

---

## 5. Tags and sets — identity, default storage, migration

This is the **hard part** of long-term compatibility. Media tiles can be
rebuilt; user tags and sets cannot.

### 5.1 Default storage (normative)

- Tags and sets (collections) live in **user.sqlite** only.
- Keys are **content identity**: canonical `blob_ref` text
  (`blob:sha256:` + lowercase hex), not paths.
- **Never** write tag/set data into the source tree by default.
- Path renames do not lose tags when the blob hash is unchanged.

Align with [TAGS.md](../TAGS.md) and dirtoo TagStore / FileSet direction.

### 5.2 Why migration is tricky

| Issue | Detail |
|-------|--------|
| **Identity is hash-based** | Export must carry `blob_ref` (and preferably raw digests), not only paths. Paths alone cannot re-attach tags after renames or cross-machine copy. |
| **tag_def catalog** | Names, labels, colors, badges are first-class. Import must merge by **name** (or stable id if we add one later), not blindly duplicate. |
| **Membership cardinality** | `blob_tag` is many tags per blob; `collection_member` is many blobs per set. Partial imports and conflicts need defined merge rules. |
| **Sources** | `source` on tags (`user` / `auto` / app id) should round-trip so auto tags are not promoted to user by accident. |
| **Orphans** | Tags for blobs that no longer exist on this machine remain valid (content may reappear). Export should not drop them; import should not require index presence. |
| **Schema drift** | User DB will gain columns over time. Export format needs a **format_version** and forward-compatible JSON (ignore unknown fields on import). |
| **dirtoo dual stores** | Until dirtoo fully uses thumtoo user DB, export/import is also the bridge between legacy TagStore/FileSet and thumtoo. |
| **No path in the primary key** | Re-import on another machine works only for content that hashes the same; path-only annotations are out of scope for the default model. |

### 5.3 JSON export / import (required capability)

Provide a documented, versioned interchange format and library/CLI support.

**Goals**

- Backup before any user-DB migration or machine move.
- Restore into a new or upgraded `user.sqlite`.
- Optional merge into an existing DB without wiping local tags/sets.
- Human-inspectable; no proprietary binary blob as the only escape hatch.

**Sketch (illustrative, not frozen wire format)**

```json
{
  "format": "thumtoo.user_overlay",
  "format_version": 1,
  "exported_at": "2026-09-17T12:00:00Z",
  "tag_defs": [
    {
      "name": "work",
      "label": "Work",
      "color": "#3B82F6",
      "badge": null,
      "created_at": 1720000000
    }
  ],
  "blob_tags": [
    {
      "blob_ref": "blob:sha256:0123…cdef",
      "tag": "work",
      "tagged_at": 1720000100,
      "source": "user"
    }
  ],
  "collections": [
    {
      "label": "Trip 2026",
      "color": null,
      "created_at": 1720000200,
      "updated_at": 1720000300,
      "members": [
        { "blob_ref": "blob:sha256:0123…cdef", "added_at": 1720000250 }
      ]
    }
  ]
}
```

**Import merge rules (v1 proposal)**

1. **tag_def:** match on `name`. If missing → insert. If present → keep existing
   label/color/badge unless import is run with an explicit “prefer import meta”
   flag.
2. **blob_tag:** match on (`blob_ref`, tag name). If missing → insert. If
   present → keep earlier `tagged_at` / `source` unless forced.
3. **collection:** match on `label` (v1; stable UUID later if needed). Members
   union by `blob_ref`.
4. Unknown JSON fields: ignore. Unknown `format_version` major: refuse with a
   clear error (do not half-apply).
5. Never require index.sqlite rows for import success.

**CLI direction (names TBD)**

- `thumtoo-user export --data-root … -o overlay.json`
- `thumtoo-user import --data-root … overlay.json [--merge|--replace-collections]`

Library API mirrors the same operations for dirtoo’s UI (“Backup tags… /
Restore…”).

### 5.4 User DB schema migration

When `user.sqlite` schema advances:

1. Run in-place SQL migrations (additive preferred).
2. If a transformation cannot be expressed in SQL alone, **export → transform →
   import** using the JSON format as the safety net.
3. Never delete `user.sqlite` as the upgrade path. If open fails, instruct the
   user to export from a previous build or from a backup copy of the file.

### 5.5 Deferred: external per-directory overlays

A future feature may support directory-local files (conceptually similar to
`.DS_Store`) that store tags/sets for that tree—for USB handoff or
git-friendly projects.

**Not default. Not required for backbone v1.**

Constraints when it is designed later:

- Opt-in; core path remains user DB only.
- Must still key membership by content hash where possible, or clearly document
  path-relative semantics and their limits.
- Must not become a second silent source of truth that diverges from the DB
  without a sync story.
- thumtoo remains responsible for schema of that file format if the library
  reads/writes it; dirtoo does not invent a private sidecar dialect.

---

## 6. Integration phases (dirtoo)

| Phase | Work |
|-------|------|
| A | Consume existing minimal snapshots; fallback to live list; write snapshot after walk |
| B | Rich stat columns + FileInfo parity; NULL-safe old rows |
| C | Watch-driven `apply_watch_delta`; stronger fingerprints |
| D | Unify MediaMetaCache / ChecksumStore / archive TOC through thumtoo |
| E | Optional FUSE read-only (not required for GUI goal) |

Tags/sets JSON export/import should land **before** any user-DB schema bump
that cannot be done with pure additive SQL, and before relying on thumtoo as
the only tag store in dirtoo.

---

## 7. Risks (summary)

| Risk | Mitigation |
|------|------------|
| Wipe-only schema bumps after release | Additive migrations; wipe is last resort |
| Tag/set loss on upgrade or machine move | Versioned JSON export/import; user DB never “delete and retry” |
| Dual identity stores during transition | Single blob_ref story; explicit bridge period |
| Treating sparse old directory rows as corrupt | Unknown fields ≠ invalid |
| Scope creep into full POSIX + xattrs + FUSE | Bound v1 fields to what dirtoo paints/filters; xattrs allowlisted |

---

## 8. Implementation checklist (library)

**Directory backbone**

- [ ] Additive directory_entry / snapshot columns; migrate_index step
- [ ] NULL-safe readers; keep `is_dir` during transition
- [ ] get listing/stat cache-only; request_refresh; warm hit without queue
- [ ] apply_watch_delta (or equivalent)
- [ ] Document fingerprint policy in schema_meta

**User overlay portability**

- [ ] Document frozen `format_version` 1 JSON (export/import)
- [ ] Library export/import with merge rules above
- [ ] CLI entry points
- [ ] Tests: round-trip, merge, orphan blob_refs, unknown fields
- [ ] User DB in-place migration path that does not delete the file

**Docs**

- [ ] Keep this file aligned with DATABASE.md when schema lands
- [ ] Note in AGENTS.md / HOST docs: post-release migration preference

---

## 9. Decision record

| Decision | Choice |
|----------|--------|
| Cache disposable by default? | **No** after release — migrate index/bulk when feasible |
| May API break? | Yes |
| Must SQLite user data survive? | **Yes** |
| Default tags/sets location | user.sqlite only; no source-tree writes |
| Portable backup for tags/sets | Versioned JSON export/import |
| External directory sidecars | Deferred feature; not default |
| Hardest long-term problem | Tag/set identity + merge across schema and machine moves |

---

## 10. Pre-release schema 101 (landed)

**Before first release**, index **101** and user **101** reserve columns and
migration so post-release upgrades stay additive.

### Index 101

- `directory_snapshot`: `inode`, `dev`, `nlink`, `mode`, `uid`, `gid`, `error_code`
- `directory_entry`: `file_type`, `mode`, `uid`, `gid`, `atime_ns`, `ctime_ns`,
  `birth_ns`, `nlink`, `inode`, `dev`, `rdev`, `symlink_target`, `blob_id`, `flags`
- `entry_xattr(dir_uri, name, key, value)` — table present; API deferred
- `kFsFileType*` constants for `file_type`
- Open path: wipe only **pre-100** caches; **100 → 101** is in-place ALTER +
  version bump (no delete)

### User 101

- `tag_def.uuid`, `collection.uuid` (nullable UNIQUE) for stable export/merge keys
- In-place migrate from user 100

### Still deferred (not required to freeze schema shape)

- Filling rich stat fields in `refresh_directory_snapshot` (lstat/xattr)
- `apply_watch_delta`, get-first listing host API polish
- JSON export/import CLI implementation
- External directory sidecars
