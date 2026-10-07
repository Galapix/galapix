// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/lod/tile_backend.hpp"
#include "thumtoo/lod/tile_types.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace thumtoo::lod {

/// One wanted cell with its issue priority (higher = sooner).
struct Demand {
  TileKey key;
  std::int64_t priority = 0;
};

struct LoaderConfig {
  /// Failed attempts before a cell waits for the cooldown instead of backoff.
  int max_attempts = 3;
  /// Backoff after the n-th failure: retry_base_ms * 2^(n-1).
  std::int64_t retry_base_ms = 500;
  /// Exhausted Failed cells are retried after this, if still demanded.
  std::int64_t failed_cooldown_ms = 30000;
  /// Queued without any reply this long ⇒ Failed (backend contract violation).
  std::int64_t stall_timeout_ms = 45000;
  /// A view's demand expires unless renewed within this window.
  std::int64_t demand_lease_ms = 4000;
  /// Ready payload budget; undemanded Ready cells are evicted LRU beyond it.
  std::size_t byte_budget = 512ull * 1024ull * 1024ull;
};

/**
 * Per-path tile loader: the **only** owner of request state for a path.
 *
 * Views (TileSession) publish *demand* (which keys, what priority); they never
 * issue or cancel. The loader turns the union of all views' demand into
 * backend requests, applies results through an explicit per-cell state
 * machine (CellState), and tells every view when cells change.
 *
 * Invariants (see docs/TILE_STATE_MACHINE.md):
 *  - At most one outstanding backend request per key (Queued + ticket).
 *    The only exception is a stall-timeout re-request.
 *  - A Queued cell leaves Queued only by a backend result for its ticket or
 *    by the stall watchdog — never by a view.
 *  - Keys no view demands are cancelled; the cell stays Queued until the
 *    backend answers (Cancelled ⇒ Missing).
 *  - Every failure keeps its reason (TileCell::error).
 *
 * Threading: results may arrive on any thread (inbox + mutex); everything else
 * is single-threaded (GUI). Time is passed in explicitly (`now_ms`) so the
 * state machine is deterministic under test.
 */
class TileLoader : public std::enable_shared_from_this<TileLoader> {
public:
  using ViewId = std::uint64_t;
  static constexpr std::int64_t kNever = std::numeric_limits<std::int64_t>::max();

  explicit TileLoader(std::shared_ptr<TileBackend> backend, LoaderConfig cfg = {});
  ~TileLoader();

  TileLoader(TileLoader const&) = delete;
  TileLoader& operator=(TileLoader const&) = delete;

  LoaderConfig const& config() const { return m_cfg; }

  /// Monotonic milliseconds used by views (default steady_clock; the
  /// scheduler installs its own clock on register, tests inject one).
  void set_clock(std::function<std::int64_t()> clock) { m_clock = std::move(clock); }
  std::int64_t now_ms() const;

  // --- Content identity -------------------------------------------------
  /**
   * Native content size defining the tile grid. The first call binds it; a
   * different size later means the content changed: every cell is dropped
   * (stale grid) and the change is recorded (size flapping = host bug).
   * @return true when cells were dropped.
   */
  bool set_content_size(int width, int height);
  int content_w() const { return m_content_w; }
  int content_h() const { return m_content_h; }
  /// Bumped by invalidate / content change; results from older epochs are dropped.
  std::uint64_t epoch() const { return m_epoch; }

  // --- Views & demand ---------------------------------------------------
  ViewId add_view(std::function<void()> on_change);
  void remove_view(ViewId id);
  /// Replace a view's demand (lease starts now).
  void set_demand(ViewId id, std::vector<Demand> demand, std::int64_t now_ms);
  /// Keep a view's current demand alive without changing it.
  void renew_demand(ViewId id, std::int64_t now_ms);
  void clear_demand(ViewId id);
  bool is_demanded(TileKey const& key) const;
  /// True while @p id holds a non-expired demand.
  bool has_demand(ViewId id) const;
  std::size_t view_count() const { return m_views.size(); }
  /// Host facts that shape every view's plan changed (e.g. a page profile
  /// arrived and moved the zoom floor): run each view's change hook.
  void notify_views();

  // --- Driven by TileScheduler (GUI thread) ------------------------------
  /// Called with a delay when the loader needs a pump (0 = asap). Thread-safe
  /// to invoke; installed by TileScheduler::register_loader.
  void set_wake(std::function<void(std::int64_t delay_ms)> wake);
  /// Delay used to coalesce demand changes during continuous zoom/pan.
  void set_demand_settle_ms(std::int64_t ms) { m_demand_settle_ms = ms; }

  /// Drain backend results into cell states. Returns results applied.
  int apply_results(std::int64_t now_ms);
  /// Lease expiry, stall watchdog, cancel of undemanded Queued, budget trim.
  void maintain(std::int64_t now_ms);

  struct Issuable {
    TileKey key;
    std::int64_t priority = 0;
  };
  /// Demanded cells that may be (re)issued now: Missing, or Failed with
  /// retry_at reached.
  void collect_issuable(std::int64_t now_ms, std::vector<Issuable>& out) const;
  /// Missing/Failed → Queued with a fresh ticket, then one backend fetch.
  void issue(std::vector<TileKey> const& keys, std::int64_t now_ms);
  /// Earliest time maintain/issue has work (retry, stall, lease); kNever if none.
  std::int64_t next_deadline_ms() const;
  int queued_count() const { return m_queued; }
  bool has_pending_results() const;

  /// Reload / file replaced: cancel outstanding work, drop every cell, bump epoch.
  void invalidate(std::string const& why);
  /// Cancel every Queued cell at the backend (cells stay Queued until the
  /// Cancelled replies). Called by the registry before dropping an idle
  /// loader; the destructor deliberately makes no backend calls (static
  /// destruction at exit may outlive thumtoo).
  void cancel_outstanding();

  // --- Reads --------------------------------------------------------------
  TileCell const* find(TileKey const& key) const;
  std::map<TileKey, TileCell> const& cells() const { return m_cells; }
  std::size_t ready_count() const { return m_ready_count; }
  std::size_t ready_bytes() const { return m_ready_bytes; }
  bool has_ready() const { return m_ready_count > 0; }
  bool has_ready_at_scale(int scale) const;
  /// Mark Ready cells as recently used (paint / visible).
  void touch(TileKey const& key);

  struct Stats {
    int queued = 0;
    int ready = 0;
    int failed = 0;
    int unavailable = 0;
    std::uint64_t issued_total = 0;
    std::uint64_t results_total = 0;
    std::uint64_t stale_results = 0;   ///< dropped (old epoch / old ticket)
    std::uint64_t stalls = 0;          ///< stall watchdog firings
    std::uint64_t cancels_sent = 0;
    std::uint64_t evicted = 0;
    int content_changes = 0;
    std::string last_error;            ///< "s=1 3,4: reason" of the latest failure
    std::string content_change_note;   ///< last "WxH → W'xH'"
  };
  Stats stats() const;

private:
  struct ViewState {
    std::function<void()> on_change;
    std::vector<Demand> demand;
    std::int64_t lease_until_ms = 0;
  };

  struct Inbox {
    std::mutex mu;
    std::vector<std::pair<std::uint64_t, FetchResult>> pending;  // (epoch, result)
    std::atomic<bool> alive{true};
    std::function<void(std::int64_t)> wake;  // guarded by mu
  };

  void rebuild_union();
  void set_cell_ready(TileKey const& key, TileBitmap bitmap, std::int64_t now_ms);
  void erase_cell(std::map<TileKey, TileCell>::iterator it);
  void fail_cell(TileCell& cell, TileKey const& key, std::string error,
                 std::int64_t now_ms);
  void request_wake(std::int64_t delay_ms);
  void trim_budget();
  void drop_all_cells_and_cancel();

  std::shared_ptr<TileBackend> m_backend;
  LoaderConfig m_cfg;
  std::function<std::int64_t()> m_clock;
  std::shared_ptr<Inbox> m_inbox;
  std::int64_t m_demand_settle_ms = 24;

  int m_content_w = 0;
  int m_content_h = 0;
  std::uint64_t m_epoch = 1;
  std::uint64_t m_next_ticket = 1;
  std::uint64_t m_lru_clock = 0;

  std::map<TileKey, TileCell> m_cells;
  int m_queued = 0;
  std::size_t m_ready_count = 0;
  std::size_t m_ready_bytes = 0;
  /// Queued keys a cancel was already sent for (cleared on any result).
  std::map<TileKey, bool> m_cancel_sent;

  ViewId m_next_view = 1;
  std::unordered_map<ViewId, ViewState> m_views;
  bool m_union_dirty = true;
  std::map<TileKey, std::int64_t> m_union;  // key → max priority

  Stats m_stats;
};

}  // namespace thumtoo::lod

