<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Full source code audit (galapix + thumtoo)

## Checklist

### Phase 0 — Docs
- [x] AGENTS.md / TODO tip / GLOSSARY skim via prior sessions

### Phase 1 — thumtoo integration (galapix)
- [x] thumtoo_callback_queue, uri, tile_provider, image_overview

### Phase 2 — Tile path
- [x] image_tile_cache, image_renderer, image, tile_provider

### Phase 3 — Viewer / workspace
- [x] viewer, viewer_command, workspace, options

### Phase 4 — Job system
- [x] job_handle, job_manager, job_worker_thread, thread, thread_message_queue2, job, jobs/*

### Phase 5 — util / math / cmake
- [x] weak_functor.hpp (OK — weak_ptr callback)
- [x] thread_pool (same non-atomic shutdown pattern as JobWorkerThread)
- [x] async_messenger (m_quit non-atomic; sleep poll)
- [x] CMake HAVE_THUMTOO / WITH_THUMTOO wiring
- [x] math/* (headers; no concurrency)

### Phase 6 — thumtoo library
- [x] ensure_lqip / get_lqip / store_lqip
- [x] executor, uri, database lqip columns
- [x] request_tile / handle_ensure_tiles (PDF live path present)
- [x] drain / worker_main / enqueue
- [x] request_size / request_pixels (structure)
- [ ] blob_store deep dive (optional)
- [ ] full pdf.cpp / archive.cpp (optional)

### Phase 7 — Cross-cutting
- [x] Dead code: `m_lqip_tried` in ImageOverview (never set true; only cleared)
- [x] Abort paths: JobHandle + cancel_jobs + overview clear on leave screen
- [x] Budget: begin_frame_request_budget / try_consume; cache hits free
- [x] GL: upload only in process_queue / overview process on main

## Findings (actionable)

### A. Thread safety (Galapix)
1. **JobHandle** — `is_aborted`/`is_finished`/`is_failed` read without mutex
2. **JobWorkerThread** — `m_quit`/`m_abort` non-atomic
3. **Viewer::redraw** — `m_mark_for_redraw` non-atomic; workers call redraw
4. **Viewer::~Viewer** — does not clear `current_`
5. **ThreadPool / AsyncMessenger** — same non-atomic shutdown flags (lower priority)

### B. LQIP / purple placeholders
- Path audited end-to-end; no proven race on same-frame process.
- `ensure_lqip` on GUI + VIPS; file/archive only; needs content_id.
- Dead `m_lqip_tried` leftover from pre-retry design.
- Runtime confirmation still needed for encode miss vs content_id miss.

### C. Architecture
- thumtoo Executor default inline; Galapix depends on queue-only callbacks.
- Prefetch max_scale grid tile may still compete with overview budget (minor).

## Proposed commits (not yet applied)
1. Atomize JobHandle flags + JobWorkerThread quit/abort + Viewer redraw/current_
2. Remove dead `m_lqip_tried`
3. Optional ensure_lqip debug logs / PDF LQIP in thumtoo

