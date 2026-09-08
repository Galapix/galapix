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

## Findings applied (continued)
6. Archive batch extract-always → warm get_tile first (thumtoo-049)
7. EnsurePixels / ProbeSize warm skip extract (thumtoo-050)
8. ThreadPool / AsyncMessenger / DownloadManager / SpaceNavigator atomics (galapix-027)
9. Handsum LQIP + C decode TU in external/ (thumtoo-046…048)
10. Levels after grid budget (galapix-025); LQIP-only skip_grid fix (galapix-024)

## Open
- [ ] blob_store deep dive (optional)
- [ ] full pdf.cpp / archive.cpp (optional)
- [ ] Overview state machine rewrite (optional)
- [ ] PDF LQIP

