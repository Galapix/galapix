<!--
SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Tile LOD core (`thumtoo::lod`)

Host-side, toolkit-free logic for showing grid tiles of a zoomable view:
which cells to load, the state of every cell, who issues requests, and what
to draw while cells are missing. It sits on top of
`Client::request_tile_cells` (TILES.md, "Interactive cell contract") and is
shared by biltoo and Galapix.

Headers: `include/thumtoo/lod/`, sources: `src/lod/`, tests:
`tests/test_lod.cpp` (deterministic clock, fake backend) and
`tests/test_lod_client_backend.cpp` (real Client).

The module moved here from biltoo (`src/tilelod/`). Its design history and
the audit of the bugs it replaced (stuck InFlight, coarse forever, re-issue
storms) are in biltoo `docs/TILE_STATE_MACHINE.md` and `docs/TILE_LOD.md`.

## Geometry (same as thumtoo tiles)

| Symbol | Meaning |
|--------|---------|
| `kTileSize` | 256, exclusive cells, edge cells smaller |
| `scale` | 0 = full resolution, +1 halves both sides, negative = denser (document pages) |
| `dim_at_tile_scale(n, s)` | successive integer floor-half, **not** `ceil(n / 2^s)` |
| grid at `s` | `ceil(dim(W,s) / 256) × ceil(dim(H,s) / 256)` |
| `max_scale_for_size` | smallest `s` with a 1×1 grid |

Hosts must take grid sizes from `lod_math.hpp`; recomputing them with other
rounding asks for cells outside the grid (Unavailable) and misplaces edge
tiles.

## Components

```text
 views                      one per source               process
 ─────                      ──────────────               ───────
 TileSession ─┐          ┌──────────────┐          ┌───────────────┐
 TileSession ─┼─ demand ─▶  TileLoader  ◀── pump ──│ TileScheduler │
 (passive) ───┘  draw    │  CellState   │          │ (sole issuer) │
                         └──────┬───────┘          └───────────────┘
                                │ fetch / cancel
                         ┌──────▼────────────┐
                         │ TileBackend       │  ClientTileBackend:
                         │ (exactly once)    │  request_tile_cells
                         └───────────────────┘
```

| Class | Owns | Never does |
|-------|------|------------|
| `TileSession` (per view) | plan (target scale, visible keys), demand, draw plan, `Phase` | issue, cancel, pump |
| `TileLoader` (per source) | per-cell `TileCell` + ticket, union of demand, results inbox, epoch, byte budget | decide global order |
| `TileScheduler` (process) | issue order across loaders, caps, wake timing | per-cell state |
| `TileBackend` | async fetch/cancel, exactly one `FetchResult` per cell | state |

`ClientTileBackend(client, uri)` is the thumtoo implementation. It decodes
every Ok cell (durable JPEG or live rgb888) to RGBA8 on the delivering
thread; `decode_tile_bitmap()` is the same decode for other callers.

## Cell states

```text
  Missing ──issue──▶ Queued ──Ok──────────▶ Ready
  (no entry)          │  ├──Cancelled──────▶ Missing   (re-issued if demanded)
                      │  ├──Failed─────────▶ Failed    (attempts++, retry_at)
                      │  ├──Unavailable────▶ Unavailable (permanent)
                      │  └──stall timeout──▶ Failed    ("no reply after N ms")
  Failed ──retry_at reached & demanded──▶ Queued
```

1. One outstanding request per key; a `Queued` cell carries a ticket and only
   a result for that ticket (or the stall watchdog) moves it. Ok results are
   accepted for the current epoch even after a stall re-request.
2. Views never touch cells. Cancel = undemanded: `maintain()` cancels Queued
   keys no view demands; the cell stays Queued until the backend answers
   Cancelled.
3. Retry with backoff (`retry_base_ms · 2^(n-1)`), after `max_attempts` a
   cooldown (`failed_cooldown_ms`). Unavailable is never retried.
4. Stall watchdog (`stall_timeout_ms`): a Queued cell without any reply
   becomes Failed with "backend contract violation" and logs
   `thumtoo/lod: STALL`. With `ClientTileBackend` it should never fire.
5. `invalidate()` and a content-size change drop all cells and bump the
   epoch, late results are dropped.
6. Ready cells beyond `byte_budget` that no view demands are evicted LRU.

## Demand

- Target scale from screen density (`target_scale_for_density`), clamped to
  `[min_scale, max_scale]`, no hold or progressive gating.
- Demanded levels: target plus `kOverviewLevels` (2) coarser levels, coarse
  first, so the first pixels are an overview.
- Priority = `class·10¹² + coarseness·10⁹ − distance to the view centre`;
  `set_priority_class()` 0 (speculative) … 3 (focused view).
- Demand is a **lease** (`demand_lease_ms`): a visible view renews it
  (`set_viewport()` / `renew()`), a view that disappears silently stops
  renewing and its cells get cancelled.
- Passive sessions plan and draw but publish no demand.

## Draw plan

Per visible key: exact Ready cell → finest Ready coarser parent (UV
sub-rect) → `Underlay` (host paints LQIP when `set_has_lqip(true)`) → empty.
Parents are never demanded just for fallback.

## Host wiring

```cpp
using namespace thumtoo::lod;

// once: scheduler wake → GUI-thread timer that calls pump()
TileScheduler::instance().set_wake_hook([](std::int64_t delay_ms) {
  post_to_gui_after(delay_ms, [] { TileScheduler::instance().pump(); });
});

// per source (share one loader between all views of the same uri)
auto loader = std::make_shared<TileLoader>(
  std::make_shared<ClientTileBackend>(client, uri));
TileScheduler::instance().register_loader(loader);

// per view
TileSession session(loader);
session.set_on_change([] { request_repaint(); });
session.set_content_size(w, h, min_scale);  // native size, from get_size()
session.set_viewport({content_rect, device_per_content});  // every frame
for (DrawCommand const& cmd : session.draw_plan().commands) { … }
```

Threading: backend results may arrive on any thread (loader inbox under a
mutex, wake hook called from that thread). Everything else, including
`pump()`, runs on one thread. Time comes from the scheduler clock
(`set_clock`), tests inject their own.

Debug: `TileScheduler::Config::trace` or `THUMTOO_LOD_DEBUG=1` prints
`thumtoo/lod-sched:` for every pump that applied or issued work;
`TileSession::status_line()` / `first_error()` / `debug_snapshot()` carry
the backend's reasons.
