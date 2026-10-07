<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Tags — thumtoo ↔ dirtoo

## Identity

Tags are keyed by **file content hash**, not by path, so renames do not drop
labels when the bytes are unchanged.

| System | Content key | Store location |
|--------|-------------|----------------|
| **dirtoo** `TagStore` | bare SHA-256 hex (64 chars) | `$XDG_DATA_HOME/dirtoo/tags.sqlite` |
| **thumtoo** | `blob_ref` = `blob:sha256:` + hex | `$XDG_STATE_HOME/thumtoo/user.sqlite` (default `data_root`) |

Wire form used inside thumtoo:

```text
blob:sha256:<64 lowercase hex>
```

Provisional or unhashed locators must not receive durable tags; hash (or
promote) first.

## Schema (thumtoo user DB)

```text
tag_def(id, uuid?, name UNIQUE, label, color, badge, created_at)
blob_tag(blob_ref, tag_id, tagged_at, source)
collection / collection_member  — sets of blob_ref
```

`uuid` on `tag_def` / `collection` (schema ≥ 101) is optional and intended for
stable export/import. Catalog metadata (colour, badge) lives here so hosts do
not need a second definition store for basic UI.

## API

```text
Client::get_tags(uri) -> [tag…]
Client::add_tag(uri, tag, source="user") -> bool
Client::remove_tag(uri, tag) -> bool

Store::ensure_tag_def / add_blob_tag / tags_for_blob_ref / …
```

`source` is free text (`user`, `auto`, app id).

## Interop with dirtoo

- Map bare hex ↔ `blob:sha256:` + hex.
- Long term: one user overlay and JSON export/import (see
  [docs/DIRTOO_BACKBONE.md](docs/DIRTOO_BACKBONE.md)).
- Until then, dirtoo may keep its own TagStore; avoid dual-writing without a
  clear merge story.

## Non-goals

- Value-bearing or namespaced tags (`ns:key=value`) until needed.
- Tagging by path alone when a content hash is known.
- Writing tag files into the source tree by default.
