// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/lod/tile_loader.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace thumtoo::lod {

/**
 * Process-wide tile request scheduler — the **only** place that issues
 * backend requests.
 *
 * One pump: for every live TileLoader apply results → maintain (leases, stall
 * watchdog, cancel undemanded, budget) → collect issuable cells → issue the
 * highest priorities across all paths within the global in-flight cap → arm
 * the next wake at the earliest loader deadline.
 *
 * Event-driven, no polling: loaders request a pump when results arrive
 * (delay 0) or demand changes (settle delay); the host's wake hook turns that
 * into a GUI-thread timer. Qt-free (tests drive pump() directly).
 */
class TileScheduler {
public:
  struct Config {
    /// Outstanding cells across all paths (≈ thumtoo workers × a few).
    int max_queued_global = 48;
    /// Outstanding cells for one path (keeps one huge image from starving others).
    int max_queued_per_loader = 32;
    /// Cells per backend fetch (one thumtoo job). Small → parallel workers.
    int chunk = 4;
    /// Coalesce demand changes (continuous zoom / pan) before issuing.
    std::int64_t demand_settle_ms = 24;
    /// Log every pump that applied or issued cells to stderr (also on with
    /// env THUMTOO_LOD_DEBUG=1).
    bool trace = false;
  };

  static TileScheduler& instance();

  TileScheduler() = default;
  TileScheduler(TileScheduler const&) = delete;
  TileScheduler& operator=(TileScheduler const&) = delete;

  void set_config(Config cfg) { m_cfg = cfg; }
  Config const& config() const { return m_cfg; }

  /// Host timer: called (any thread) with the delay until the next pump().
  void set_wake_hook(std::function<void(std::int64_t delay_ms)> hook);
  /// Monotonic milliseconds (default: steady_clock).
  void set_clock(std::function<std::int64_t()> clock);
  std::int64_t now_ms() const;

  /// Track a loader (weak) and install its wake.
  void register_loader(std::shared_ptr<TileLoader> const& loader);

  /// Ask for a pump after @p delay_ms (thread-safe).
  void request_pump(std::int64_t delay_ms);

  /// GUI thread: one full scheduling pass.
  void pump();

  struct Stats {
    int loaders = 0;
    int queued = 0;
    int issuable = 0;           ///< demanded cells waiting for capacity / retry
    std::uint64_t pumps = 0;
    std::uint64_t issued_total = 0;
  };
  Stats stats() const;

private:
  std::vector<std::shared_ptr<TileLoader>> live_loaders();

  Config m_cfg;
  mutable std::mutex m_mu;  // guards m_hook, m_clock, m_loaders
  std::function<void(std::int64_t)> m_hook;
  std::function<std::int64_t()> m_clock;
  std::vector<std::weak_ptr<TileLoader>> m_loaders;
  Stats m_stats;
};

}  // namespace thumtoo::lod

