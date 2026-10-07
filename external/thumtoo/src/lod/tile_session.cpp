// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/lod/tile_session.hpp"

#include "thumtoo/lod/lod_math.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

namespace thumtoo::lod {

namespace {

// Priority layout (higher issues first):
//   class (view importance) ≫ coarseness (overview before detail) ≫ distance.
constexpr std::int64_t kClassWeight = 1'000'000'000'000LL;
constexpr std::int64_t kLevelWeight = 1'000'000'000LL;
constexpr std::int64_t kMaxDistance = kLevelWeight - 1;

}  // namespace

TileSession::TileSession(std::shared_ptr<TileLoader> loader)
    : m_loader(std::move(loader))
{
  std::weak_ptr<bool> alive = m_alive;
  m_view = m_loader->add_view([this, alive]() {
    if (auto a = alive.lock(); a && *a) {
      on_loader_change();
    }
  });
}

TileSession::~TileSession()
{
  *m_alive = false;
  m_loader->remove_view(m_view);
}

void TileSession::on_loader_change()
{
  m_draw_plan_dirty = true;
  if (m_on_change) {
    m_on_change();
  }
}

void TileSession::set_priority_class(int cls)
{
  cls = std::clamp(cls, 0, 9);
  if (cls == m_priority_class) {
    return;
  }
  m_priority_class = cls;
  publish_demand();
}

void TileSession::set_passive(bool on)
{
  if (on == m_passive) {
    return;
  }
  m_passive = on;
  if (m_passive) {
    clear_demand();
  } else {
    publish_demand();
  }
}

void TileSession::set_content_size(int width, int height, int min_scale)
{
  if (width <= 0 || height <= 0) {
    return;
  }
  // Binding first: a content change in the loader drops cells for every view.
  (void)m_loader->set_content_size(width, height);
  if (m_content_w == width && m_content_h == height && m_min_scale == min_scale) {
    return;
  }
  m_content_w = width;
  m_content_h = height;
  m_min_scale = min_scale;
  m_max_scale = std::max(max_scale_for_size(width, height), m_min_scale);
  m_draw_plan_dirty = true;
  // Geometry changed: the plan must follow now, not on the next viewport
  // change (static viewports used to keep the old floor forever).
  if (m_have_viewport) {
    replan();
  }
}

void TileSession::set_viewport(Viewport const& vp, double margin_content)
{
  m_viewport = vp;
  m_margin = margin_content;
  m_have_viewport = true;
  replan();
}

void TileSession::replan()
{
  if (m_content_w <= 0 || m_content_h <= 0) {
    return;
  }
  PlannerInput in;
  in.content_w = m_content_w;
  in.content_h = m_content_h;
  in.min_scale = m_min_scale;
  in.max_scale = m_max_scale;
  in.viewport = m_viewport;
  in.margin_content = m_margin;
  PlannerOutput out = plan_visible_tiles(in);
  bool const changed =
      out.target_scale != m_target_scale || out.visible_keys != m_visible_keys;
  if (changed) {
    m_target_scale = out.target_scale;
    m_visible_keys = std::move(out.visible_keys);
    ++m_generation;
    m_draw_plan_dirty = true;
    publish_demand();
  } else {
    renew();
  }
}

std::vector<Demand> TileSession::build_demand() const
{
  std::vector<Demand> out;
  if (m_content_w <= 0 || m_content_h <= 0 || m_visible_keys.empty()) {
    return out;
  }
  int const t = m_target_scale;
  // Levels with their coarseness rank (higher rank = issued earlier).
  std::vector<std::pair<int, int>> levels;  // (scale, rank)
  // Same rule for every target, denser document scales included: thumtoo
  // renders any level per cell from one shared display list and one image
  // decode, so intermediate levels are cheap overview.
  int const top = std::min(m_max_scale, t + kOverviewLevels);
  for (int s = top; s >= t; --s) {
    levels.emplace_back(s, s - t);
  }

  double const cx = m_viewport.content_rect.x + m_viewport.content_rect.w * 0.5;
  double const cy = m_viewport.content_rect.y + m_viewport.content_rect.h * 0.5;
  std::int64_t const cls = static_cast<std::int64_t>(m_priority_class) * kClassWeight;

  std::set<TileKey> seen;
  for (auto const& [scale, rank] : levels) {
    std::vector<TileKey> keys;
    if (scale == t) {
      keys = m_visible_keys;
    } else {
      PlannerInput in;
      in.content_w = m_content_w;
      in.content_h = m_content_h;
      in.min_scale = scale;
      in.max_scale = scale;
      in.viewport = m_viewport;
      in.margin_content = m_margin;
      keys = plan_visible_tiles(in).visible_keys;
    }
    for (TileKey const& k : keys) {
      if (!seen.insert(k).second) {
        continue;
      }
      RectI const cr = tile_content_rect(m_content_w, m_content_h, k);
      double const dx = cr.x + cr.w * 0.5 - cx;
      double const dy = cr.y + cr.h * 0.5 - cy;
      auto const dist = static_cast<std::int64_t>(std::sqrt(dx * dx + dy * dy));
      std::int64_t const prio = cls + static_cast<std::int64_t>(rank) * kLevelWeight
                                + (kMaxDistance - std::min(dist, kMaxDistance));
      out.push_back({k, prio});
    }
  }
  return out;
}

void TileSession::publish_demand()
{
  if (m_passive) {
    return;
  }
  std::vector<Demand> d = build_demand();
  bool same = m_demand_published && d.size() == m_demand.size();
  if (same) {
    for (std::size_t i = 0; i < d.size(); ++i) {
      if (d[i].key != m_demand[i].key || d[i].priority != m_demand[i].priority) {
        same = false;
        break;
      }
    }
  }
  if (same) {
    m_loader->renew_demand(m_view, m_loader->now_ms());
    return;
  }
  m_demand = d;
  m_demand_published = true;
  m_loader->set_demand(m_view, std::move(d), m_loader->now_ms());
}

void TileSession::renew()
{
  if (m_passive) {
    return;
  }
  if (!m_demand_published) {
    publish_demand();
    return;
  }
  // The lease may have lapsed (view not renewed for a while): re-publish so
  // the loader has it again; otherwise just extend.
  if (!m_demand.empty() && !m_loader->has_demand(m_view)) {
    m_loader->set_demand(m_view, m_demand, m_loader->now_ms());
    return;
  }
  m_loader->renew_demand(m_view, m_loader->now_ms());
}

void TileSession::clear_demand()
{
  m_demand.clear();
  m_demand_published = false;
  m_loader->clear_demand(m_view);
}

DrawPlan TileSession::draw_plan() const
{
  if (!m_draw_plan_dirty) {
    return m_draw_plan_cache;
  }
  BuildDrawPlanInput in;
  in.content_w = m_content_w;
  in.content_h = m_content_h;
  in.target_scale = m_target_scale;
  in.max_scale = m_max_scale;
  in.visible_keys = m_visible_keys;
  in.has_lqip = m_has_lqip;
  TileLoader const* loader = m_loader.get();
  in.lookup = [loader](TileKey const& k) { return loader->find(k); };
  m_draw_plan_cache = build_draw_plan(in);
  m_draw_plan_dirty = false;
  return m_draw_plan_cache;
}

TileSession::Coverage TileSession::coverage() const
{
  Coverage c;
  c.visible = static_cast<int>(m_visible_keys.size());
  int const max_attempts = m_loader->config().max_attempts;
  for (TileKey const& key : m_visible_keys) {
    TileCell const* e = m_loader->find(key);
    bool has_pixels = false;
    if (!e) {
      ++c.missing;
    } else {
      switch (e->state) {
      case CellState::Ready:
        if (e->ready()) {
          ++c.ready;
          has_pixels = true;
        } else {
          ++c.missing;
        }
        break;
      case CellState::Queued:
        ++c.queued;
        break;
      case CellState::Failed:
        if (e->attempts < max_attempts) {
          ++c.retrying;
        } else {
          ++c.failed;
        }
        break;
      case CellState::Unavailable:
        ++c.unavailable;
        break;
      }
    }
    if (!has_pixels) {
      bool parent = false;
      for (int d = 1; key.scale + d <= m_max_scale; ++d) {
        TileCell const* p = m_loader->find(parent_key(key, d));
        if (p && p->ready()) {
          parent = true;
          break;
        }
      }
      if (!parent) {
        ++c.holes;
      }
    }
  }
  return c;
}

TileSession::Phase TileSession::phase() const
{
  if (m_content_w <= 0 || m_visible_keys.empty()) {
    return Phase::Idle;
  }
  Coverage const c = coverage();
  if (c.fully_covered()) {
    return Phase::Complete;
  }
  if (c.loading()) {
    return Phase::Loading;
  }
  return c.holes > 0 ? Phase::Error : Phase::Degraded;
}

char const* TileSession::phase_name(Phase p)
{
  switch (p) {
  case Phase::Idle:
    return "idle";
  case Phase::Loading:
    return "loading";
  case Phase::Complete:
    return "complete";
  case Phase::Degraded:
    return "degraded";
  case Phase::Error:
    return "error";
  }
  return "?";
}

std::string TileSession::first_error() const
{
  for (TileKey const& key : m_visible_keys) {
    TileCell const* e = m_loader->find(key);
    if (e && (e->state == CellState::Failed || e->state == CellState::Unavailable)
        && !e->error.empty()) {
      char buf[48];
      std::snprintf(buf, sizeof buf, "s=%d %d,%d: ", key.scale, key.x, key.y);
      return buf + e->error;
    }
  }
  return {};
}

std::string TileSession::status_line() const
{
  Coverage const c = coverage();
  Phase const p = phase();
  char buf[160];
  switch (p) {
  case Phase::Idle:
    return "tiles idle";
  case Phase::Complete:
    std::snprintf(buf, sizeof buf, "tiles s=%d complete (%d)", m_target_scale,
                  c.visible);
    return buf;
  case Phase::Loading:
    std::snprintf(buf, sizeof buf, "tiles s=%d loading %d/%d (queued %d, retry %d)",
                  m_target_scale, c.ready, c.visible, c.queued, c.retrying);
    return buf;
  case Phase::Degraded:
  case Phase::Error:
    std::snprintf(buf, sizeof buf, "tiles s=%d %s: %d/%d ready, %d failed, %d unavailable — ",
                  m_target_scale, phase_name(p), c.ready, c.visible, c.failed,
                  c.unavailable);
    return buf + first_error();
  }
  return {};
}

TileSession::DebugSnapshot TileSession::debug_snapshot() const
{
  DebugSnapshot s;
  s.content_w = m_content_w;
  s.content_h = m_content_h;
  s.target_scale = m_target_scale;
  s.min_scale = m_min_scale;
  s.max_scale = m_max_scale;
  s.cov = coverage();
  s.phase = phase();
  s.demand = static_cast<int>(m_demand.size());
  s.has_lqip = m_has_lqip;
  s.generation = m_generation;
  s.loader = m_loader->stats();
  DrawPlan const plan = draw_plan();
  for (DrawCommand const& cmd : plan.commands) {
    switch (cmd.kind) {
    case DrawKind::ExactTile:
      ++s.plan_exact;
      break;
    case DrawKind::CoarserTile:
      ++s.plan_parent;
      break;
    case DrawKind::Underlay:
      ++s.plan_underlay;
      break;
    case DrawKind::Empty:
      ++s.plan_empty;
      break;
    }
  }
  int const commanded = s.plan_exact + s.plan_parent + s.plan_underlay + s.plan_empty;
  if (s.cov.visible > commanded) {
    s.plan_empty += s.cov.visible - commanded;
  }
  return s;
}

}  // namespace thumtoo::lod
