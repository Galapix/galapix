<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Full source code audit (galapix + thumtoo)

## Checklist

### Phase 0–5 — Done
- [x] Docs, thumtoo integration, tile path, viewer/workspace, job system
- [x] util (weak_functor, thread_pool, async_messenger), math, CMake HAVE_THUMTOO

### Phase 6 — thumtoo
- [x] ensure_lqip / get_lqip / store_lqip / executor / uri / db lqip
- [x] request_tile / handle_ensure_tiles (PDF live rgb888) / drain / worker
- [x] request_size / request_pixels (structure)
- [ ] blob_store deep dive (optional follow-up)
- [ ] full pdf.cpp / archive.cpp (optional follow-up)

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

## Open
- LQIP purple: runtime confirm with --debug (see TODO tip galapix-107/108)
- Optional: blob_store, ThreadPool shutdown atomics, Executor GUI marshal

