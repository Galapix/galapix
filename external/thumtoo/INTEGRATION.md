<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# biltoo ↔ thumtoo integration

Primary consumer of the size index and display ladder. thumtoo keys are
**source URIs** only; biltoo **SessionImageId** stays session/edit identity and
must not key durable pixels or tags.

**Pixel durability / archives:** [docs/PIXEL_AND_ARCHIVE_POLICY.md](docs/PIXEL_AND_ARCHIVE_POLICY.md) (tiles + LQIP durable; soft ephemeral; random vs sequential archives).

## Mapping

| biltoo need | thumtoo API |
|-------------|-------------|
| Replace provisional layout size (`imageSizeForPath` / probe) | `get_size(uri)` then `request_size(uri, cb)` |
| Whole-image soft (ephemeral) / TileSynth | `get_pixels` / `request_pixels` ≤ **512**; prefer tiles for display |
| High-res on zoom (Gallery) | Consumer **full decode** or `request_tile` — not `request_pixels(1024+)` |
| Deep zoom / region | `get_tile` / `request_tile` ([TILES.md](TILES.md)) |
| Known-good meta without I/O | `get_meta(uri)` (status, format, still_count, …) |
| Prewarm session paths | `prepare_paths` or CLI `thumtoo-prepare` |
| Archive open without re-walk | `refresh_archive_toc` / `get_archive_entries` |
| Image inside zip/cbz/… | URI `file:///…//archive:member` + normal get/request size/pixels |

| Drop cached pixels / text for a URI | `Client::purge_uri` (also clears `page_text_layer` even when the document blob is shared) |
| Drop all cache rows for a file path | `Client::purge_path` |
| OCR / native page text | `request_page_text` / Store text layers (requires MuPDF and/or Tesseract at build time) |

Suggested `max_edge` starting points (tunable in biltoo):

| Mode | max_edge | Notes |
|------|----------|--------|
| Filmstrip / grid cell | 128 or 256 | Soft only |
| Gallery overview | tiles or ephemeral ≤512 | Prefer `request_tile`; soft not durable |
| Image-mode soft underlay | 512 | Cap at soft max; full decode for native |
| Anything larger on screen | — | `request_tile` or app full decode |

`request_pixels` clamps to 512. Prefer **largest cached soft level ≤ request**;
do not treat a 256 reply as success for a 512 request — ensure must upgrade.

## Threading (Qt)

```text
Client::open(cache_root, Executor{[](auto fn) {
  QMetaObject::invokeMethod(qApp, std::move(fn), Qt::QueuedConnection);
}});
```

- GUI thread: only `get_*` / schedule `request_*`
- Callbacks arrive on the Qt event loop via the Executor
- Never decode or hash on the GUI thread

## URI form

- File: `file:///absolute/path.jpg` (`thumtoo::file_uri_from_path`)
- Archive member (Location form): `file:///book.zip//archive:inner/path.jpg`

biltoo path strings should be converted once at the boundary; do not pass
relative paths into the cache.

## Cache root

Default: `$XDG_CACHE_HOME/thumtoo` (or `~/.cache/thumtoo`). Tests may pass an
explicit root. biltoo should not write beside source images.

## Status handling

| `ContentStatus` | UI hint |
|-----------------|---------|
| `pending` / `incomplete` | Placeholder; keep request in flight |
| `ready` | Use size / pixels |
| `failed` | Allow retry later |
| `unsupported` | Stop retrying this locator |

## Out of scope for thumtoo

- Crop / flip / session edits → biltoo `SessionImageId`
- Desktop icon thumbnailers → Freedesktop Thumbnailer1 (dirtoo client)
- Live directory watching → dirtoo (thumtoo only stores optional snapshots)

## Tags

Tags are optional and **content-keyed** (`sha256:…`). See [TAGS.md](TAGS.md) for
dirtoo `TagStore` alignment. biltoo should not invent path-only tags; resolve
URI → content via `get_meta` / locator before writing labels.

