<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Pixel pipeline (thumtoo)

Host-facing design lives in biltoo:

[`docs/PIXEL_PIPELINE_REDESIGN.md`](https://github.com/Grumbel/biltoo/blob/master/docs/PIXEL_PIPELINE_REDESIGN.md)

## Mapped onto existing thumtoo APIs

| Plan concept | thumtoo today |
|--------------|---------------|
| Soft ≤512 (ephemeral) | `get_pixels` / `request_pixels`; not Store-durable — see PIXEL_AND_ARCHIVE_POLICY.md |
| FastBatch ≤1024 | `kBatchMaxEdge`, `kBatchWindowMembers`, **`request_overview_pixels`** |
| Archive cursor | `ArchiveCursor`, `plan_archive_batch_window`, Client worker uses TOC-ordered window |
| Tile pyramid | `get_tile` / `request_tile` / `request_tile_pyramid`, [TILES.md](../TILES.md) |
| Construct ladder from tiles | **`get_pixels_from_tiles`** (`PixelSource::TileSynth`) |
| Quality | `PixelSource`, `TileSource` |
| Focus full | full decode → `request_tile_pyramid` |

## Archive cursor (Phase 2 partial)

- **One logical cursor per archive path** on `Client` (`archive_cursors_`).
- TOC is loaded once (image members only) via `ensure_archive_cursor`.
- `plan_archive_batch_window(ordered, interest, next_index, max_window)` is pure:
  returns a TOC-ordered contiguous window ≤ `kBatchWindowMembers`.
- Worker same-archive coalesce path plans the extract set through the cursor and
  advances `next_index` past the last extracted TOC index after a successful
  libarchive pass.
- Process extract LRU (existing) remains the shared member-byte cache for focus.

Shipped beyond the table: `set_interest`, interest epoch cancel, FastScale
`request_overview_pixels`, FocusFull + caps, Q1 invalidate on pyramid,
`request_raster` / `get_raster`, `queue_stats()`.

## Phase status

- [x] Tile pyramid (durable) + ephemeral soft / TileSynth
- [x] `PixelSource::TileSynth` + `get_pixels_from_tiles` (construct)
- [x] Archive cursor + TOC-ordered windowed batch extract (worker path)
- [x] Interest epoch + `bump_interest_epoch` / `cancel_pending` / `cancel_uri`
- [x] `set_interest` snapshot (overview + Primary → tile pyramid FocusFull)
- [x] FastScale Q1 via `request_overview_pixels` (≤ kBatchMaxEdge, JpegShrink store)
- [x] Unified `request_raster` / `get_raster` (RasterPolicy)

## Archive access class

See [PIXEL_AND_ARCHIVE_POLICY.md](PIXEL_AND_ARCHIVE_POLICY.md) §2.

- **Random** (typical ZIP): extract-on-demand + LRU.
- **Sequential** (tar, solid RAR/7z): one cursor, windowed forward extract, prepare in one pass.
