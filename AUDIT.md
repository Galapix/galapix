<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Full source code audit (galapix + thumtoo)

## Checklist

### Phase 0–5 — Done
- [x] Docs, thumtoo integration, tile path, viewer/workspace, job system
- [x] util (weak_functor, thread_pool, async_messenger atomics), math, CMake HAVE_THUMTOO

### Phase 6 — thumtoo
- [x] ensure_lqip / get_lqip / store_lqip / executor / uri / db lqip
- [x] request_tile / handle_ensure_tiles (PDF live rgb888) / drain / worker
- [x] request_size / request_pixels (structure)
- [x] blob_store deep dive (2026-09-08)
- [x] pdf.cpp / archive.cpp structure pass (2026-09-08)

### Phase 7 — Cross-cutting
- [x] Dead m_lqip_tried removed
- [x] JobHandle / JobWorkerThread / Viewer atomics (galapix-021)
- [x] LQIP debug + one-shot miss log (galapix-022/023)
- [x] Budget / abort / GL upload on main

## Findings applied
1. JobHandle atomic flags
2. JobWorkerThread atomic quit/abort
3. Viewer redraw atomic CAS + clear current_
4. Remove m_lqip_tried
5. LQIP path log_debug (decode / fail / upload / one-shot miss)

## Findings applied (continued)
6. Archive batch extract-always → warm get_tile first (thumtoo-049)
7. EnsurePixels / ProbeSize warm skip extract (thumtoo-050)
8. ThreadPool / AsyncMessenger / DownloadManager / SpaceNavigator atomics (galapix-027)
9. Handsum LQIP + C decode TU in external/ (thumtoo-046…048)
10. Levels after grid budget (galapix-025); LQIP-only skip_grid fix (galapix-024)

## BlobStore / Database (checked)

**blob_store.cpp / .hpp**
- WAL + busy_timeout=5000 — good for multi-process
- Schema: level_blobs, tile_blobs, http_bodies; sensible PKs
- No connection mutex: Client defaults to N=hardware_concurrency workers
  sharing one sqlite3* for db_ and blobs_. Relies on SQLite SERIALIZED
  mode (not UB, but contention under load). Main thread also calls
  ensure_lqip / get_size / get_tile_coverage on the same connections.
- prepare_v2 every get/put (no statement cache) — fine for correctness;
  optional perf win later
- SQLITE_STATIC binds OK while caller owns buffers through step
- No blob size limits on put — large ladder levels can grow cache without bound

**database.cpp** (spot check with client)
- Same multi-thread sharing model as BlobStore
- LQIP on content row; meta_for_uri joins locator→content

**archive.cpp**
- Sequential libarchive walk; RAR/ZIP extract cost is inherent
- Batch extract_archive_members used by Client; warm path now skips
  extract when tiles/levels/size cached (thumtoo-049/050)
- extract_cache_ is process-RAM only (mutex protected)

**pdf.cpp**
- Poppler optional; page URI //page:N
- Layout size + rasterize for tiles; no LQIP/ThumbHash/Handsum path
- Live negative-scale tiles still a separate Galapix concern

## Overview state (ImageOverview) — rewrite candidate

Current flags: State + m_lqip_only + m_levels_requested + m_lqip_miss_logged.
Works after galapix-024/025 but remains easy to break (Loading + LQIP
vs levels failure restore). Prefer explicit phases if touched again:
`Idle → LqipReady → LevelsLoading → LevelsReady | Failed`.

## Open
- [x] Serialize Database/BlobStore with recursive_mutex (thumtoo-051)
- [ ] Optional: prepared statement cache in BlobStore
- [x] Overview: Underlay enum orthogonal to State (galapix-031)
- [ ] PDF LQIP
- [ ] Blob cache eviction / size budget

### Non-thumtoo overview (TileGenerator)

`OverviewLoadJob` → `TileGenerator::load_surface` uses **libjpeg DCT scale**
`jpeg_scale = min(pow2(min_scale), 8)` for JPEG files. Thumtoo interactive
tiles do not currently match this (full decode + RAM ladder); see thumtoo
`docs/THUMBNAIL_AUDIT.md` §6d and §8.1.

## GUI thread I/O (sleeping USB / spin-up)

Source volumes may take **10–30s** to respond after idle. The GUI thread must
not touch source paths (or even assume SQLite on a slow disk is free).

| Call | Thread | Notes |
|------|--------|-------|
| `Viewer::open_paths` expand (stat, PDF/DjVu count, archive TOC) | **worker** | Results applied in `process_pending_opens` |
| `ThumtooTileProvider::create` (`request_size`+`drain`) | **forbidden on GUI** | open_paths uses cache `get_size` only + SizeProbe |
| `ImageOverview` `get_lqip` | GUI, **≥250ms backoff** on miss | SQLite is still I/O |
| `get_size` / `get_tile` cache hits | prefer worker; GUI OK for local XDG cache | Keep thumtoo cache on fast disk |

Never put `$XDG_CACHE_HOME/thumtoo` on the same USB as the photo library if
that volume spins down.

### LQIP (no display special-case)

LQIP is written on the worker **after** the first durable thumbnail/tile. On a
cold open it is therefore absent while better content paints; successive opens
find it already in the DB. Galapix only **reads** `get_lqip` — never suppresses
it with warm/cold session flags.

