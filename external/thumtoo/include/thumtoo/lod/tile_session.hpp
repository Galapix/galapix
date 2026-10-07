// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/lod/draw_plan.hpp"
#include "thumtoo/lod/lod_planner.hpp"
#include "thumtoo/lod/tile_loader.hpp"
#include "thumtoo/lod/tile_types.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace thumtoo::lod {

/**
 * One view surface onto a path's tiles (ImageItem, filmstrip cell, slideshow
 * phase, neighbour prefetch). Qt-free.
 *
 * Responsibilities — and nothing else:
 *  1. Plan: viewport + density → target scale + visible keys.
 *  2. Demand: publish wanted keys (target scale + coarser overview levels,
 *     coarse first) to the shared TileLoader. Never issues, never cancels.
 *  3. Draw: exact → finest Ready parent → hole, from the loader's cells.
 *  4. Status: an explicit Phase with the failure reason, so a stuck or failed
 *     cell is reported instead of silently staying coarse.
 */
class TileSession {
public:
  explicit TileSession(std::shared_ptr<TileLoader> loader);
  ~TileSession();

  TileSession(TileSession const&) = delete;
  TileSession& operator=(TileSession const&) = delete;

  /// Called (GUI thread) whenever cells this view may draw changed.
  void set_on_change(std::function<void()> cb) { m_on_change = std::move(cb); }

  /// Issue priority class (0 = speculative … 3 = focused view).
  void set_priority_class(int cls);
  int priority_class() const { return m_priority_class; }

  /// Passive views plan and draw but never publish demand (paint-only peeks).
  void set_passive(bool on);
  bool passive() const { return m_passive; }

  /**
   * Grid geometry. Binds/changes the loader's content size (a change drops
   * the path's cells — see TileLoader::set_content_size).
   */
  void set_content_size(int width, int height, int min_scale = 0);
  int content_w() const { return m_content_w; }
  int content_h() const { return m_content_h; }
  int min_scale() const { return m_min_scale; }
  int max_scale() const { return m_max_scale; }

  void set_has_lqip(bool on)
  {
    if (m_has_lqip != on) {
      m_has_lqip = on;
      m_draw_plan_dirty = true;
    }
  }
  bool has_lqip() const { return m_has_lqip; }

  /**
   * Re-plan for a viewport and publish demand when the plan changed; renew
   * the demand lease otherwise. Cheap to call every paint.
   */
  void set_viewport(Viewport const& vp, double margin_content = 0.0);
  /// Keep the current demand alive (static viewport while cells load).
  void renew();
  /// Withdraw demand (view hidden); the plan is kept for drawing.
  void clear_demand();

  int target_scale() const { return m_target_scale; }
  std::vector<TileKey> const& visible_keys() const { return m_visible_keys; }
  /// Bumped whenever the plan (target scale / visible keys) changes.
  std::uint64_t generation() const { return m_generation; }
  /// Demand published to the loader (for tests / debug).
  std::vector<Demand> const& demand() const { return m_demand; }

  DrawPlan draw_plan() const;

  TileLoader& loader() { return *m_loader; }
  TileLoader const& loader() const { return *m_loader; }
  TileCell const* find(TileKey const& key) const { return m_loader->find(key); }

  bool has_any_ready() const { return m_loader->has_ready(); }

  /// Exact-scale coverage of the visible keys.
  struct Coverage {
    int visible = 0;
    int ready = 0;
    int queued = 0;
    int missing = 0;
    int retrying = 0;     ///< Failed, retry still pending (attempts < max)
    int failed = 0;       ///< Failed, retries exhausted (cooldown)
    int unavailable = 0;
    int holes = 0;        ///< no exact and no Ready parent: nothing to draw
    bool fully_covered() const { return visible > 0 && ready >= visible; }
    /// Work left that will change this view without user action.
    bool loading() const { return queued + missing + retrying > 0; }
  };
  Coverage coverage() const;

  enum class Phase {
    Idle,      ///< No content / nothing visible
    Loading,   ///< Cells queued, missing or retrying
    Complete,  ///< Every visible cell at the target scale is Ready
    Degraded,  ///< Done; some cells failed but parents cover them
    Error,     ///< Done; some visible area has no pixels at all
  };
  Phase phase() const;
  static char const* phase_name(Phase p);
  /// First failure reason among visible cells ("" when none).
  std::string first_error() const;
  /// One line for HUD / logs: "tiles s=0 loading 12/20" / "tiles: 3 failed — …".
  std::string status_line() const;

  struct DebugSnapshot {
    int content_w = 0;
    int content_h = 0;
    int target_scale = 0;
    int min_scale = 0;
    int max_scale = 0;
    Coverage cov;
    Phase phase = Phase::Idle;
    int demand = 0;
    int plan_exact = 0;
    int plan_parent = 0;
    int plan_underlay = 0;
    int plan_empty = 0;
    bool has_lqip = false;
    std::uint64_t generation = 0;
    TileLoader::Stats loader;
  };
  DebugSnapshot debug_snapshot() const;

  /// Coarser levels demanded above the target for raster scales (≥ 0).
  static constexpr int kOverviewLevels = 2;

private:
  void on_loader_change();
  void replan();
  void publish_demand();
  std::vector<Demand> build_demand() const;

  std::shared_ptr<TileLoader> m_loader;
  TileLoader::ViewId m_view = 0;
  std::function<void()> m_on_change;
  /// Guards the loader→view callback against a view destroyed mid-notify.
  std::shared_ptr<bool> m_alive = std::make_shared<bool>(true);
  int m_priority_class = 2;
  bool m_passive = false;

  int m_content_w = 0;
  int m_content_h = 0;
  int m_min_scale = 0;
  int m_max_scale = 0;
  bool m_has_lqip = false;

  Viewport m_viewport;
  double m_margin = 0.0;
  bool m_have_viewport = false;
  int m_target_scale = 0;
  std::vector<TileKey> m_visible_keys;
  std::uint64_t m_generation = 0;
  std::vector<Demand> m_demand;
  bool m_demand_published = false;

  mutable bool m_draw_plan_dirty = true;
  mutable DrawPlan m_draw_plan_cache;
};

}  // namespace thumtoo::lod

