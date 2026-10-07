// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// Qt-free unit tests for the thumtoo::lod core: math, planner, draw plan, and the
/// explicit tile state machine (TileLoader / TileScheduler / TileSession).
/// docs/TILE_STATE_MACHINE.md lists the bug each state-machine test pins.

#include "thumtoo/lod/draw_plan.hpp"
#include "thumtoo/lod/lod_math.hpp"
#include "thumtoo/lod/lod_planner.hpp"
#include "thumtoo/lod/tile_backend.hpp"
#include "thumtoo/lod/tile_loader.hpp"
#include "thumtoo/lod/tile_scheduler.hpp"
#include "thumtoo/lod/tile_session.hpp"
#include "thumtoo/lod/source_records.hpp"
#include "thumtoo/lod/source_status.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

#define CHECK(cond)                                                            \
  do {                                                                         \
    if (!(cond)) {                                                             \
      std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #cond      \
                << "\n";                                                       \
      ++g_failures;                                                            \
    }                                                                          \
  } while (0)

#define CHECK_EQ(a, b)                                                         \
  do {                                                                         \
    auto _va = (a);                                                            \
    auto _vb = (b);                                                            \
    if (_va != _vb) {                                                          \
      std::cerr << "FAIL " << __FILE__ << ":" << __LINE__ << " " << #a         \
                << " == " << #b << "  (" << _va << " != " << _vb << ")\n";     \
      ++g_failures;                                                            \
    }                                                                          \
  } while (0)

using thumtoo::lod::CellState;
using thumtoo::lod::FetchStatus;
using thumtoo::lod::TileKey;
using Phase = thumtoo::lod::TileSession::Phase;

thumtoo::lod::TileBitmap solid_tile(int w, int h, std::uint8_t r)
{
  thumtoo::lod::TileBitmap b;
  b.width = w;
  b.height = h;
  b.codec = "rgba8";
  b.bytes.assign(static_cast<size_t>(w * h * 4), 0);
  for (int i = 0; i < w * h; ++i) {
    b.bytes[static_cast<size_t>(i * 4 + 0)] = r;
    b.bytes[static_cast<size_t>(i * 4 + 3)] = 255;
  }
  return b;
}

thumtoo::lod::TileCell ready_cell(int w = 256, int h = 256)
{
  thumtoo::lod::TileCell c;
  c.state = CellState::Ready;
  c.bitmap = solid_tile(w, h, 10);
  return c;
}

/**
 * Contract-abiding fake: every fetched cell stays outstanding until the test
 * answers it; cancel answers Cancelled (like thumtoo request_tile_cells).
 * `honour_cancel = false` models a broken backend for the stall watchdog.
 */
class FakeBackend : public thumtoo::lod::TileBackend {
public:
  struct Outstanding {
    thumtoo::lod::FetchRequest req;
    ResultFn cb;
  };
  std::vector<Outstanding> outstanding;
  std::vector<std::vector<TileKey>> fetch_calls;
  std::vector<TileKey> cancelled;
  bool honour_cancel = true;

  void fetch(std::vector<thumtoo::lod::FetchRequest> cells, ResultFn on_result) override
  {
    std::vector<TileKey> keys;
    for (auto const& c : cells) {
      keys.push_back(c.key);
      outstanding.push_back({c, on_result});
    }
    fetch_calls.push_back(keys);
  }

  void cancel(std::vector<TileKey> const& keys) override
  {
    for (auto const& k : keys) {
      cancelled.push_back(k);
      if (honour_cancel) {
        (void)answer(k, FetchStatus::Cancelled, "cancelled by host");
      }
    }
  }

  bool has_outstanding(TileKey const& k) const
  {
    return std::any_of(outstanding.begin(), outstanding.end(),
                       [&](Outstanding const& o) { return o.req.key == k; });
  }

  int fetched_count(TileKey const& k) const
  {
    int n = 0;
    for (auto const& call : fetch_calls) {
      n += static_cast<int>(std::count(call.begin(), call.end(), k));
    }
    return n;
  }

  int total_fetched() const
  {
    int n = 0;
    for (auto const& call : fetch_calls) {
      n += static_cast<int>(call.size());
    }
    return n;
  }

  /// Answer the oldest outstanding request for @p k.
  bool answer(TileKey const& k, FetchStatus st, std::string err = {})
  {
    for (auto it = outstanding.begin(); it != outstanding.end(); ++it) {
      if (it->req.key == k) {
        Outstanding o = *it;
        outstanding.erase(it);
        thumtoo::lod::FetchResult r;
        r.key = o.req.key;
        r.ticket = o.req.ticket;
        r.status = st;
        r.error = std::move(err);
        if (st == FetchStatus::Ok) {
          r.bitmap = solid_tile(thumtoo::lod::kTileSize, thumtoo::lod::kTileSize, 99);
        }
        o.cb(std::move(r));
        return true;
      }
    }
    return false;
  }

  int answer_all(FetchStatus st = FetchStatus::Ok, std::string err = {})
  {
    int n = 0;
    while (!outstanding.empty()) {
      TileKey const k = outstanding.front().req.key;
      answer(k, st, err);
      ++n;
    }
    return n;
  }
};

/// Scheduler + loader with a manual clock; the wake hook only records.
struct Rig {
  std::int64_t now = 10'000;
  std::shared_ptr<FakeBackend> backend = std::make_shared<FakeBackend>();
  thumtoo::lod::TileScheduler sched;
  std::shared_ptr<thumtoo::lod::TileLoader> loader;
  std::vector<std::int64_t> wakes;

  explicit Rig(thumtoo::lod::LoaderConfig cfg = {}, thumtoo::lod::TileScheduler::Config sc = {})
  {
    sched.set_config(sc);
    sched.set_clock([this]() { return now; });
    sched.set_wake_hook([this](std::int64_t d) { wakes.push_back(d); });
    loader = std::make_shared<thumtoo::lod::TileLoader>(backend, cfg);
    sched.register_loader(loader);
  }

  std::shared_ptr<thumtoo::lod::TileLoader> add_loader()
  {
    auto l = std::make_shared<thumtoo::lod::TileLoader>(backend);
    sched.register_loader(l);
    return l;
  }

  void pump() { sched.pump(); }

  /// Pump, answer everything Ok, pump — until nothing is outstanding.
  void run_until_idle(int rounds = 16)
  {
    for (int i = 0; i < rounds; ++i) {
      pump();
      if (backend->outstanding.empty()) {
        pump();
        if (backend->outstanding.empty()) {
          return;
        }
      }
      backend->answer_all();
    }
  }
};

thumtoo::lod::Viewport full_view(int w, int h, double dpc)
{
  thumtoo::lod::Viewport vp;
  vp.content_rect = {0, 0, static_cast<double>(w), static_cast<double>(h)};
  vp.device_per_content = dpc;
  return vp;
}

}  // namespace

namespace {
void test_dim_at_tile_scale()
{
  CHECK_EQ(thumtoo::lod::dim_at_tile_scale(1000, 0), 1000);
  CHECK_EQ(thumtoo::lod::dim_at_tile_scale(1000, 1), 500);
  CHECK_EQ(thumtoo::lod::dim_at_tile_scale(1000, 2), 250);
  CHECK_EQ(thumtoo::lod::dim_at_tile_scale(1000, 3), 125);
  CHECK_EQ(thumtoo::lod::dim_at_tile_scale(5, 1), 2);
  CHECK_EQ(thumtoo::lod::dim_at_tile_scale(5, 2), 1);
  CHECK_EQ(thumtoo::lod::dim_at_tile_scale(5, 3), 0);
  // Negative: exact expand (PDF live denser tiles).
  CHECK_EQ(thumtoo::lod::dim_at_tile_scale(1000, -1), 2000);
  CHECK_EQ(thumtoo::lod::dim_at_tile_scale(1000, -2), 4000);
  CHECK_EQ(thumtoo::lod::dim_at_tile_scale(256, -1), 512);
}

void test_negative_scale_content_rect()
{
  // Layout 512×512, scale -1 → level 1024; cell (0,0) content maps to 128×128.
  auto r = thumtoo::lod::tile_content_rect(512, 512, { -1, 0, 0 });
  CHECK(!r.empty());
  CHECK_EQ(r.x, 0);
  CHECK_EQ(r.y, 0);
  CHECK_EQ(r.w, 128);
  CHECK_EQ(r.h, 128);
  // tiles_across at -1 for 512: level 1024 → 4 tiles
  CHECK_EQ(thumtoo::lod::tiles_across(512, -1), 4);
}

void test_planner_negative_scale_stride()
{
  // Content 512×512, scale −1: 4×4 cells of 128 content px. Full-page query
  // must list all 16 keys (old step=256 only produced a 2×2 subset).
  thumtoo::lod::PlannerInput in;
  in.content_w = 512;
  in.content_h = 512;
  in.min_scale = -4;
  in.max_scale = 4;
  in.viewport.content_rect = {0, 0, 512, 512};
  in.viewport.device_per_content = 2.0; // target scale −1
  in.margin_content = 0;
  auto out = thumtoo::lod::plan_visible_tiles(in);
  CHECK_EQ(out.target_scale, -1);
  CHECK_EQ(static_cast<int>(out.visible_keys.size()), 16);
}

void test_density_allows_negative_min()
{
  // Zoom denser than 1:1 with document floor -4.
  int s = thumtoo::lod::target_scale_for_density(2.0, thumtoo::lod::kDocumentLiveMinScale, 8);
  CHECK(s < 0);
  // Raster floor 0 still clamps.
  CHECK_EQ(thumtoo::lod::target_scale_for_density(2.0, 0, 8), 0);
}

void test_max_scale_and_grid()
{
  // 512×512 → scale 0: 2×2 tiles; scale 1: 1×1
  CHECK_EQ(thumtoo::lod::tiles_across(512, 0), 2);
  CHECK_EQ(thumtoo::lod::tiles_across(512, 1), 1);
  CHECK_EQ(thumtoo::lod::max_scale_for_size(512, 512), 1);

  // 256×256 → already one tile at scale 0
  CHECK_EQ(thumtoo::lod::max_scale_for_size(256, 256), 0);

  // 257×10 → scale 0: 2×1; need scale until both 1
  CHECK_EQ(thumtoo::lod::tiles_across(257, 0), 2);
  CHECK(thumtoo::lod::max_scale_for_size(257, 10) >= 1);
}

void test_planner_1to1()
{
  thumtoo::lod::PlannerInput in;
  in.content_w = 512;
  in.content_h = 512;
  in.min_scale = 0;
  in.max_scale = thumtoo::lod::max_scale_for_size(512, 512);
  in.viewport.content_rect = {0, 0, 512, 512};
  in.viewport.device_per_content = 1.0;

  auto out = thumtoo::lod::plan_visible_tiles(in);
  CHECK_EQ(out.target_scale, 0);
  CHECK_EQ(static_cast<int>(out.visible_keys.size()), 4);  // 2×2
}

void test_planner_zoomed_out()
{
  thumtoo::lod::PlannerInput in;
  in.content_w = 2048;
  in.content_h = 2048;
  in.min_scale = 0;
  in.max_scale = thumtoo::lod::max_scale_for_size(2048, 2048);
  in.viewport.content_rect = {0, 0, 2048, 2048};
  in.viewport.device_per_content = 0.25;  // heavily zoomed out

  auto out = thumtoo::lod::plan_visible_tiles(in);
  CHECK(out.target_scale > 0);
  // Coarser scale → fewer cells than scale 0 (8×8 = 64)
  CHECK(static_cast<int>(out.visible_keys.size()) < 64);
}

void test_planner_partial_viewport()
{
  thumtoo::lod::PlannerInput in;
  in.content_w = 1024;
  in.content_h = 1024;
  in.min_scale = 0;
  in.max_scale = thumtoo::lod::max_scale_for_size(1024, 1024);
  // Only top-left 200×200 content pixels at 1:1
  in.viewport.content_rect = {0, 0, 200, 200};
  in.viewport.device_per_content = 1.0;

  auto out = thumtoo::lod::plan_visible_tiles(in);
  CHECK_EQ(out.target_scale, 0);
  // One tile cell covers 256×256 content at scale 0
  CHECK_EQ(static_cast<int>(out.visible_keys.size()), 1);
  CHECK_EQ(out.visible_keys[0].x, 0);
  CHECK_EQ(out.visible_keys[0].y, 0);
}

void test_parent_uv_edge_tile()
{
  // Non power-of-two content: uniform span UV drifts; content-rect mapping must
  // place the fine cell at the correct fraction of the parent.
  thumtoo::lod::TileKey fine{0, 3, 0};
  thumtoo::lod::TileKey parent{1, 1, 0};
  int const content_w = 1000;
  int const content_h = 1000;
  int const parent_px = 244; // typical edge payload
  auto uv = thumtoo::lod::parent_uv_for_child(fine, parent, parent_px, parent_px,
                                         content_w, content_h);
  auto fine_cr = thumtoo::lod::tile_content_rect(content_w, content_h, fine);
  auto parent_cr = thumtoo::lod::tile_content_rect(content_w, content_h, parent);
  CHECK(!fine_cr.empty());
  CHECK(!parent_cr.empty());
  // UV origin ≈ (fine.x - parent.x) / parent.w * parent_px
  double expect_u = (static_cast<double>(fine_cr.x - parent_cr.x)
                     / static_cast<double>(parent_cr.w))
                    * parent_px;
  CHECK(uv.x + 1e-6 >= expect_u - 1e-3);
  CHECK(uv.x <= expect_u + 1e-3);
  CHECK(uv.w > 0 && uv.h > 0);
  // Must not be the full parent (that was the "repeated whole tile" symptom).
  CHECK(uv.w < parent_px - 1e-3 || uv.h < parent_px - 1e-3 || uv.x > 1e-3);
}

void test_edge_tile_content_rect()
{
  // 300×100: scale 0 tiles (0,0) covers 256×100; (1,0) covers 44×100
  auto r0 = thumtoo::lod::tile_content_rect(300, 100, {0, 0, 0});
  CHECK_EQ(r0.w, 256);
  CHECK_EQ(r0.h, 100);
  auto r1 = thumtoo::lod::tile_content_rect(300, 100, {0, 1, 0});
  CHECK_EQ(r1.x, 256);
  CHECK_EQ(r1.w, 44);
}

/// Content mapping: tiles abut; last column stretches to native edge so
/// total coverage equals content_w (no floor-half residual ring).
void test_content_coverage_odd_widths()
{
  int const widths[] = {300, 513, 1025, 2052, 4097};
  for (int cw : widths) {
    for (int s = 0; s < 6; ++s) {
      int const nx = thumtoo::lod::tiles_across(cw, s);
      if (nx <= 0) {
        continue;
      }
      int const factor = 1 << s;
      int covered = 0;
      int prev_right = 0;
      for (int x = 0; x < nx; ++x) {
        auto r = thumtoo::lod::tile_content_rect(cw, cw, {s, x, 0});
        CHECK(!r.empty());
        CHECK_EQ(r.x, prev_right);
        // Interior tiles stay level*2^s (multiple of factor). Trailing edge
        // may stretch by a floor-half residual and need not be.
        if (x + 1 < nx) {
          CHECK_EQ(r.w % factor, 0);
        }
        covered += r.w;
        prev_right = r.x + r.w;
      }
      CHECK_EQ(covered, cw);
    }
  }
  CHECK(thumtoo::lod::tile_content_rect(513, 513, {1, 99, 0}).empty());
}

void test_parent_key()
{
  auto p = thumtoo::lod::parent_key({0, 3, 5}, 1);
  CHECK_EQ(p.scale, 1);
  CHECK_EQ(p.x, 1);
  CHECK_EQ(p.y, 2);
  p = thumtoo::lod::parent_key({0, 3, 5}, 2);
  CHECK_EQ(p.scale, 2);
  CHECK_EQ(p.x, 0);
  CHECK_EQ(p.y, 1);
}

// ---------------------------------------------------------------------------
// Draw plan (pure)
// ---------------------------------------------------------------------------

struct CellMap {
  std::map<TileKey, thumtoo::lod::TileCell> cells;
  thumtoo::lod::TileLookup lookup()
  {
    return [this](TileKey const& k) -> thumtoo::lod::TileCell const* {
      auto it = cells.find(k);
      return it == cells.end() ? nullptr : &it->second;
    };
  }
};

void test_fallback_parent_uv()
{
  CellMap m;
  m.cells[{1, 0, 0}] = ready_cell();
  thumtoo::lod::BuildDrawPlanInput in;
  in.content_w = 512;
  in.content_h = 512;
  in.target_scale = 0;
  in.max_scale = 1;
  in.visible_keys = {{0, 0, 0}, {0, 1, 0}};
  in.lookup = m.lookup();
  auto plan = thumtoo::lod::build_draw_plan(in);
  CHECK_EQ(static_cast<int>(plan.commands.size()), 2);
  CHECK(plan.any_tile);
  for (auto const& c : plan.commands) {
    CHECK(c.kind == thumtoo::lod::DrawKind::CoarserTile);
    CHECK_EQ(c.src_key.scale, 1);
    CHECK(c.src_uv.w > 0 && c.src_uv.h > 0);
  }
  CHECK(plan.commands[0].src_uv.x < plan.commands[1].src_uv.x);
}

void test_lqip_until_tile()
{
  CellMap m;
  thumtoo::lod::BuildDrawPlanInput in;
  in.content_w = 256;
  in.content_h = 256;
  in.visible_keys = {{0, 0, 0}};
  in.has_lqip = true;
  in.lookup = m.lookup();
  auto plan = thumtoo::lod::build_draw_plan(in);
  CHECK_EQ(static_cast<int>(plan.commands.size()), 1);
  CHECK(plan.commands[0].kind == thumtoo::lod::DrawKind::Underlay);
  CHECK(!plan.any_tile);
  m.cells[{0, 0, 0}] = ready_cell();
  plan = thumtoo::lod::build_draw_plan(in);
  CHECK(plan.commands[0].kind == thumtoo::lod::DrawKind::ExactTile);
}

/// Failed / Queued / Unavailable exact all fall back to a Ready parent.
void test_non_ready_exact_falls_to_parent()
{
  for (CellState st : {CellState::Failed, CellState::Queued, CellState::Unavailable}) {
    CellMap m;
    thumtoo::lod::TileCell c;
    c.state = st;
    m.cells[{0, 0, 0}] = c;
    m.cells[{1, 0, 0}] = ready_cell();
    thumtoo::lod::BuildDrawPlanInput in;
    in.content_w = 512;
    in.content_h = 512;
    in.max_scale = 1;
    in.visible_keys = {{0, 0, 0}};
    in.lookup = m.lookup();
    auto plan = thumtoo::lod::build_draw_plan(in);
    CHECK_EQ(static_cast<int>(plan.commands.size()), 1);
    CHECK(plan.commands[0].kind == thumtoo::lod::DrawKind::CoarserTile);
  }
}

void test_empty_plan_without_lqip()
{
  CellMap m;
  thumtoo::lod::BuildDrawPlanInput in;
  in.content_w = 256;
  in.content_h = 256;
  in.visible_keys = {{0, 0, 0}};
  in.lookup = m.lookup();
  auto plan = thumtoo::lod::build_draw_plan(in);
  CHECK(plan.commands.empty());
  CHECK(!plan.any_tile);
}

// ---------------------------------------------------------------------------
// State machine
// ---------------------------------------------------------------------------

/// First pixels fast: overview levels are demanded and issued before detail.
void test_coarse_first_issue_order()
{
  Rig rig;
  thumtoo::lod::TileSession s(rig.loader);
  s.set_content_size(2048, 2048);
  s.set_viewport(full_view(2048, 2048, 0.5));  // target 1, max 3
  CHECK_EQ(s.target_scale(), 1);
  rig.pump();
  CHECK(!rig.backend->fetch_calls.empty());
  std::vector<int> scales;
  for (auto const& call : rig.backend->fetch_calls) {
    for (auto const& k : call) {
      scales.push_back(k.scale);
    }
  }
  CHECK_EQ(static_cast<int>(scales.size()), 1 + 4 + 16);
  CHECK_EQ(scales.front(), 3);
  CHECK(std::is_sorted(scales.rbegin(), scales.rend()));  // non-increasing
  // Chunked: no single job carries the whole viewport.
  for (auto const& call : rig.backend->fetch_calls) {
    CHECK(static_cast<int>(call.size()) <= rig.sched.config().chunk);
  }
}

/// Bug: per-session InFlight in a shared cache — one view's request blocked
/// the other, and only the issuing view could apply the completion.
void test_two_views_share_one_request_per_key()
{
  Rig rig;
  thumtoo::lod::TileSession a(rig.loader);
  thumtoo::lod::TileSession b(rig.loader);
  for (auto* s : {&a, &b}) {
    s->set_content_size(1024, 1024);
    s->set_viewport(full_view(1024, 1024, 1.0));
  }
  rig.pump();
  for (TileKey const& k : a.visible_keys()) {
    CHECK_EQ(rig.backend->fetched_count(k), 1);
  }
  rig.backend->answer_all();
  rig.pump();
  CHECK(a.phase() == Phase::Complete);
  CHECK(b.phase() == Phase::Complete);
}

/// Bug: Gallery virtual paint built a throwaway controller per slot; its
/// destructor cancelled every InFlight key of the live cell.
void test_destroying_a_view_never_cancels_peer_demand()
{
  Rig rig;
  thumtoo::lod::TileSession live(rig.loader);
  live.set_content_size(1024, 1024);
  live.set_viewport(full_view(1024, 1024, 1.0));
  rig.pump();
  int const fetched = rig.backend->total_fetched();
  {
    thumtoo::lod::TileSession peek(rig.loader);
    peek.set_passive(true);
    peek.set_content_size(1024, 1024);
    peek.set_viewport(full_view(1024, 1024, 1.0));
    (void)peek.draw_plan();
  }
  {
    // Even an active peer with identical demand going away must not cancel.
    thumtoo::lod::TileSession peer(rig.loader);
    peer.set_content_size(1024, 1024);
    peer.set_viewport(full_view(1024, 1024, 1.0));
  }
  rig.pump();
  CHECK(rig.backend->cancelled.empty());
  CHECK_EQ(rig.backend->total_fetched(), fetched);
  CHECK(live.phase() == Phase::Loading);
}

/// Pan away: undemanded Queued cells are cancelled, come back Missing, and
/// are re-issued when wanted again.
void test_pan_cancels_undemanded_and_reissues()
{
  Rig rig;
  thumtoo::lod::TileSession s(rig.loader);
  s.set_content_size(2048, 256);
  thumtoo::lod::Viewport left;
  left.content_rect = {0, 0, 512, 256};
  left.device_per_content = 1.0;
  thumtoo::lod::Viewport right = left;
  right.content_rect.x = 1536;
  s.set_viewport(left);
  rig.pump();
  TileKey const k{0, 0, 0};
  CHECK(rig.backend->has_outstanding(k));
  s.set_viewport(right);
  rig.pump();  // maintain → cancel → backend answers Cancelled
  CHECK(std::count(rig.backend->cancelled.begin(), rig.backend->cancelled.end(), k) == 1);
  rig.pump();  // apply Cancelled → Missing
  CHECK(rig.loader->find(k) == nullptr);
  s.set_viewport(left);
  rig.pump();
  CHECK_EQ(rig.backend->fetched_count(k), 2);
}

/// Bug: thumtoo dropped queued tile jobs on every interest-epoch bump without
/// a callback. Cancelled must never be terminal while the key is wanted.
void test_cancelled_while_demanded_is_reissued()
{
  Rig rig;
  thumtoo::lod::TileSession s(rig.loader);
  s.set_content_size(256, 256);
  s.set_viewport(full_view(256, 256, 1.0));
  rig.pump();
  TileKey const k{0, 0, 0};
  CHECK(rig.backend->answer(k, FetchStatus::Cancelled, "external cancel"));
  rig.pump();
  CHECK_EQ(rig.backend->fetched_count(k), 2);
  CHECK(s.phase() == Phase::Loading);
}

/// Failures retry with backoff, then report the backend's reason instead of
/// sitting on a coarse stand-in; a cooldown retries transient errors.
void test_failed_backoff_then_error_with_reason()
{
  Rig rig;
  thumtoo::lod::TileSession s(rig.loader);
  s.set_content_size(256, 256);  // single cell, no parent
  s.set_viewport(full_view(256, 256, 1.0));
  TileKey const k{0, 0, 0};
  rig.pump();
  rig.backend->answer(k, FetchStatus::Failed, "decode error: boom");
  rig.pump();
  CHECK(rig.loader->find(k)->state == CellState::Failed);
  CHECK(s.phase() == Phase::Loading);  // retry pending
  rig.now += 499;
  rig.pump();
  CHECK_EQ(rig.backend->fetched_count(k), 1);
  rig.now += 1;
  rig.pump();
  CHECK_EQ(rig.backend->fetched_count(k), 2);
  rig.backend->answer(k, FetchStatus::Failed, "decode error: boom");
  rig.pump();
  rig.now += 1000;
  rig.pump();
  CHECK_EQ(rig.backend->fetched_count(k), 3);
  rig.backend->answer(k, FetchStatus::Failed, "decode error: boom");
  rig.pump();
  CHECK(s.phase() == Phase::Error);
  CHECK(s.first_error().find("boom") != std::string::npos);
  CHECK(s.status_line().find("boom") != std::string::npos);
  s.renew();
  rig.now += 29'999;
  s.renew();
  rig.pump();
  CHECK_EQ(rig.backend->fetched_count(k), 3);
  rig.now += 1;
  s.renew();
  rig.pump();
  CHECK_EQ(rig.backend->fetched_count(k), 4);
}

/// Unavailable is permanent and explained; a Ready parent keeps the view
/// Degraded (drawn) rather than Error.
void test_unavailable_is_never_retried()
{
  Rig rig;
  thumtoo::lod::TileSession s(rig.loader);
  s.set_content_size(512, 512);
  s.set_viewport(full_view(512, 512, 1.0));
  rig.pump();
  TileKey const bad{0, 1, 1};
  rig.backend->answer(bad, FetchStatus::Unavailable, "cell outside the tile grid");
  rig.backend->answer_all();
  rig.pump();
  CHECK(s.phase() == Phase::Degraded);
  CHECK(s.first_error().find("outside") != std::string::npos);
  for (int i = 0; i < 10; ++i) {
    rig.now += 10'000;
    s.renew();
    rig.pump();
  }
  CHECK_EQ(rig.backend->fetched_count(bad), 1);
}

/// A backend that never answers is reported (stall watchdog) and retried;
/// a late Ok is still accepted.
void test_stall_watchdog_reports_and_recovers()
{
  thumtoo::lod::LoaderConfig cfg;
  cfg.stall_timeout_ms = 45'000;
  cfg.demand_lease_ms = 1'000'000;
  Rig rig(cfg);
  rig.backend->honour_cancel = false;
  thumtoo::lod::TileSession s(rig.loader);
  s.set_content_size(256, 256);
  s.set_viewport(full_view(256, 256, 1.0));
  TileKey const k{0, 0, 0};
  rig.pump();
  rig.now += 44'999;
  rig.pump();
  CHECK(rig.loader->find(k)->state == CellState::Queued);
  rig.now += 1;
  rig.pump();
  CHECK(rig.loader->find(k)->state == CellState::Failed);
  CHECK(rig.loader->find(k)->error.find("no reply") != std::string::npos);
  CHECK_EQ(rig.loader->stats().stalls, 1u);
  rig.now += 500;
  rig.pump();
  CHECK_EQ(rig.backend->fetched_count(k), 2);
  rig.backend->answer(k, FetchStatus::Ok);  // the late first request
  rig.pump();
  CHECK(rig.loader->find(k)->ready());
  rig.backend->answer(k, FetchStatus::Ok);  // duplicate
  rig.pump();
  CHECK(rig.loader->find(k)->ready());
  CHECK(rig.loader->stats().stale_results >= 1u);
}

/// Results issued before invalidate (reload) never paint stale pixels.
void test_invalidate_drops_late_results()
{
  Rig rig;
  rig.backend->honour_cancel = false;
  thumtoo::lod::TileSession s(rig.loader);
  s.set_content_size(256, 256);
  s.set_viewport(full_view(256, 256, 1.0));
  rig.pump();
  rig.loader->invalidate("reload");
  rig.backend->answer_all();  // late Ok for the old content
  rig.pump();                 // applies (drops) + re-issues for the new epoch
  TileKey const k{0, 0, 0};
  CHECK(rig.loader->find(k) != nullptr);
  CHECK(rig.loader->find(k)->state == CellState::Queued);
  CHECK_EQ(rig.backend->fetched_count(k), 2);
  rig.backend->answer_all();
  rig.pump();
  CHECK(rig.loader->find(k)->ready());
}

/// A different native size drops the grid and is recorded (flapping = bug).
void test_content_size_change_drops_cells()
{
  Rig rig;
  thumtoo::lod::TileSession s(rig.loader);
  s.set_content_size(1024, 1024);
  s.set_viewport(full_view(1024, 1024, 1.0));
  rig.run_until_idle();
  CHECK(rig.loader->has_ready());
  s.set_content_size(1000, 1000);
  CHECK(!rig.loader->has_ready());
  CHECK_EQ(rig.loader->stats().content_changes, 1);
  CHECK(rig.loader->stats().content_change_note.find("1024x1024") != std::string::npos);
  rig.run_until_idle();
  CHECK(s.phase() == Phase::Complete);
}

/// Demand is a lease: a view that stops renewing releases its cells.
void test_demand_lease_expires_without_renew()
{
  thumtoo::lod::LoaderConfig cfg;
  cfg.demand_lease_ms = 4000;
  Rig rig(cfg);
  thumtoo::lod::TileSession s(rig.loader);
  s.set_content_size(256, 256);
  s.set_viewport(full_view(256, 256, 1.0));
  rig.pump();
  TileKey const k{0, 0, 0};
  for (int i = 0; i < 6; ++i) {
    rig.now += 1000;
    s.renew();
    rig.pump();
  }
  CHECK(rig.backend->cancelled.empty());
  rig.now += 4000;
  rig.pump();
  CHECK_EQ(static_cast<int>(rig.backend->cancelled.size()), 1);
  rig.pump();
  CHECK(rig.loader->find(k) == nullptr);
  s.renew();  // view visible again: lapsed demand is re-published
  rig.pump();
  CHECK_EQ(rig.backend->fetched_count(k), 2);
}

/// Global cap honours view priority across paths.
void test_priority_across_paths()
{
  thumtoo::lod::TileScheduler::Config sc;
  sc.max_queued_global = 4;
  Rig rig({}, sc);
  auto other_backend = std::make_shared<FakeBackend>();
  auto focus_loader = std::make_shared<thumtoo::lod::TileLoader>(other_backend);
  rig.sched.register_loader(focus_loader);
  thumtoo::lod::TileSession bg(rig.loader);
  bg.set_priority_class(0);
  thumtoo::lod::TileSession focus(focus_loader);
  focus.set_priority_class(3);
  for (auto* s : {&bg, &focus}) {
    s->set_content_size(1024, 1024);
    s->set_viewport(full_view(1024, 1024, 1.0));
  }
  rig.pump();
  CHECK_EQ(other_backend->total_fetched(), 4);
  CHECK_EQ(rig.backend->total_fetched(), 0);
  other_backend->answer_all();
  rig.pump();
  CHECK_EQ(other_backend->total_fetched(), 8);
}

/// Bug: draw plans were cached per session and only dirtied by that
/// session's own pump — tiles landed by a peer never repainted.
void test_draw_plan_follows_loader_changes()
{
  Rig rig;
  thumtoo::lod::TileSession s(rig.loader);
  int changes = 0;
  s.set_on_change([&]() { ++changes; });
  s.set_content_size(256, 256);
  s.set_viewport(full_view(256, 256, 1.0));
  CHECK(s.draw_plan().commands.empty());
  thumtoo::lod::TileSession other(rig.loader);
  other.set_content_size(256, 256);
  other.set_viewport(full_view(256, 256, 1.0));
  rig.pump();
  rig.backend->answer_all();
  rig.pump();
  CHECK(changes > 0);
  auto plan = s.draw_plan();
  CHECK_EQ(static_cast<int>(plan.commands.size()), 1);
  CHECK(plan.commands[0].kind == thumtoo::lod::DrawKind::ExactTile);
}

/// Ok without pixels is a contract violation → Failed with a reason.
void test_ok_without_pixels_is_a_failure()
{
  Rig rig;
  thumtoo::lod::TileSession s(rig.loader);
  s.set_content_size(256, 256);
  s.set_viewport(full_view(256, 256, 1.0));
  rig.pump();
  auto o = rig.backend->outstanding.front();
  rig.backend->outstanding.clear();
  thumtoo::lod::FetchResult r;
  r.key = o.req.key;
  r.ticket = o.req.ticket;
  r.status = FetchStatus::Ok;
  o.cb(r);
  rig.pump();
  TileKey const k{0, 0, 0};
  CHECK(rig.loader->find(k)->state == CellState::Failed);
  CHECK(!rig.loader->find(k)->error.empty());
}

/// Waiting for capacity must not spin (no zero-delay self wake).
void test_no_busy_wake_when_capacity_full()
{
  thumtoo::lod::TileScheduler::Config sc;
  sc.max_queued_global = 1;
  Rig rig({}, sc);
  thumtoo::lod::TileSession s(rig.loader);
  s.set_content_size(1024, 1024);
  s.set_viewport(full_view(1024, 1024, 1.0));
  rig.pump();
  rig.wakes.clear();
  rig.pump();
  CHECK(!rig.wakes.empty());
  for (std::int64_t w : rig.wakes) {
    CHECK(w > 0);
  }
}

/// Live denser document target: same overview rule as rasters (T+1, T+2) —
/// thumtoo renders any level per cell from one display list and one decode.
void test_document_denser_demand_levels()
{
  Rig rig;
  thumtoo::lod::TileSession s(rig.loader);
  s.set_content_size(1000, 1400, thumtoo::lod::kDocumentLiveMinScale);
  thumtoo::lod::Viewport vp;
  vp.content_rect = {0, 0, 300, 300};
  vp.device_per_content = 4.0;
  s.set_viewport(vp);
  CHECK_EQ(s.target_scale(), -2);
  std::vector<int> scales;
  for (auto const& d : s.demand()) {
    scales.push_back(d.key.scale);
  }
  std::sort(scales.begin(), scales.end());
  scales.erase(std::unique(scales.begin(), scales.end()), scales.end());
  CHECK(scales == (std::vector<int>{-2, -1, 0}));
}

/// Two views wanting the same key: one leaving does not cancel it.
void test_shared_key_survives_one_view_leaving()
{
  Rig rig;
  thumtoo::lod::TileSession a(rig.loader);
  thumtoo::lod::TileSession b(rig.loader);
  a.set_content_size(2048, 256);
  b.set_content_size(2048, 256);
  thumtoo::lod::Viewport left;
  left.content_rect = {0, 0, 512, 256};
  left.device_per_content = 1.0;
  a.set_viewport(left);
  b.set_viewport(left);
  rig.pump();
  thumtoo::lod::Viewport right = left;
  right.content_rect.x = 1536;
  b.set_viewport(right);
  rig.pump();
  TileKey const k{0, 0, 0};
  CHECK(std::count(rig.backend->cancelled.begin(), rig.backend->cancelled.end(), k) == 0);
  CHECK(rig.backend->has_outstanding(k));
}

/// Min-scale floor change re-plans immediately (no viewport change needed).
void test_min_scale_change_replans_static_view()
{
  Rig rig;
  thumtoo::lod::TileSession s(rig.loader);
  s.set_content_size(4096, 4096, 3);
  s.set_viewport(full_view(4096, 4096, 1.0));
  CHECK_EQ(s.target_scale(), 3);
  auto const gen = s.generation();
  s.set_content_size(4096, 4096, 0);
  CHECK_EQ(s.target_scale(), 0);
  CHECK(s.generation() != gen);
}

}  // namespace

thumtoo::PdfPageProfile raster_profile(double dpi, int cap)
{
  thumtoo::PdfPageProfile p;
  p.kind = thumtoo::PageContentKind::Raster;
  p.native_dpi = dpi;
  p.finest_useful_scale = cap;
  p.image_draws = 1;
  p.summary = "raster: test";
  return p;
}

/// Zoom floor policy: images 0; PDF pages provisional 0 until the profile is
/// known; raster pages at their cap; vector/mixed at the document floor.
void test_zoom_floor_policy()
{
  using thumtoo::lod::ProfileState;
  using thumtoo::lod::SourceKind;
  int const doc = thumtoo::lod::kDocumentLiveMinScale;
  thumtoo::lod::SourceRecord img;
  img.kind = SourceKind::Image;
  CHECK_EQ(thumtoo::lod::decide_zoom_floor(img, doc).min_scale, 0);

  thumtoo::lod::SourceRecord pdf;
  pdf.kind = SourceKind::PdfPage;
  pdf.profile_state = ProfileState::Pending;
  auto f = thumtoo::lod::decide_zoom_floor(pdf, doc);
  CHECK_EQ(f.min_scale, 0);
  CHECK(f.provisional);
  CHECK(f.reason.find("pending") != std::string::npos);

  pdf.profile_state = ProfileState::Known;
  pdf.profile = raster_profile(300, -1);
  f = thumtoo::lod::decide_zoom_floor(pdf, doc);
  CHECK_EQ(f.min_scale, -1);
  CHECK(!f.provisional);
  CHECK(f.reason.find("300 dpi") != std::string::npos);

  pdf.profile = raster_profile(5000, -6);  // finer than the document floor allows
  CHECK_EQ(thumtoo::lod::decide_zoom_floor(pdf, doc).min_scale, doc);

  pdf.profile->kind = thumtoo::PageContentKind::Mixed;
  pdf.profile->finest_useful_scale.reset();
  f = thumtoo::lod::decide_zoom_floor(pdf, doc);
  CHECK_EQ(f.min_scale, doc);
  CHECK(f.reason.find("mixed") != std::string::npos);

  pdf.profile_state = ProfileState::Failed;
  pdf.profile_error = "cannot open";
  f = thumtoo::lod::decide_zoom_floor(pdf, doc);
  CHECK_EQ(f.min_scale, 0);
  CHECK(f.reason.find("cannot open") != std::string::npos);

  thumtoo::lod::SourceRecord djvu;
  djvu.kind = SourceKind::DjvuPage;
  djvu.profile_state = ProfileState::Pending;
  CHECK_EQ(thumtoo::lod::decide_zoom_floor(djvu, doc).min_scale, 0);
  CHECK(thumtoo::lod::decide_zoom_floor(djvu, doc).provisional);
  djvu.profile_state = ProfileState::Known;
  djvu.profile = raster_profile(300, 0);
  f = thumtoo::lod::decide_zoom_floor(djvu, doc);
  CHECK_EQ(f.min_scale, 0);
  CHECK(!f.provisional);
  CHECK(f.reason.find("native pixels") != std::string::npos);

  thumtoo::lod::SourceRecord epub;
  epub.kind = SourceKind::EpubPage;
  epub.profile_state = ProfileState::Pending;
  CHECK_EQ(thumtoo::lod::decide_zoom_floor(epub, doc).min_scale, 0);
  epub.profile_state = ProfileState::Known;
  epub.profile = raster_profile(111, 0);  // full-page cover image
  CHECK_EQ(thumtoo::lod::decide_zoom_floor(epub, doc).min_scale, 0);
  epub.profile->kind = thumtoo::PageContentKind::Vector;
  epub.profile->finest_useful_scale.reset();
  CHECK_EQ(thumtoo::lod::decide_zoom_floor(epub, doc).min_scale, doc);
}

/// Decisions are keyed by `what`; unchanged values do not bump generation.
void test_source_record_decisions()
{
  auto& recs = thumtoo::lod::SourceRecords::instance();
  recs.clear();
  recs.ensure("/a.pdf//page:1", thumtoo::lod::SourceKind::PdfPage);
  auto const g0 = recs.generation();
  CHECK(recs.decide("/a.pdf//page:1", "zoom floor", "scale 0", "pending", 1));
  CHECK(!recs.decide("/a.pdf//page:1", "zoom floor", "scale 0", "pending", 2));
  CHECK(recs.decide("/a.pdf//page:1", "zoom floor", "scale -1", "raster", 3));
  CHECK(recs.generation() > g0);
  auto const* r = recs.find("/a.pdf//page:1");
  CHECK(r && r->decisions.size() == 1);
  CHECK(r && r->decision("zoom floor") && r->decision("zoom floor")->value == "scale -1");
  CHECK(!recs.decide("/missing", "x", "y", "z", 4));
  CHECK(recs.decide("/a.pdf//page:1", "gallery floor", "scale 2", "small cell", 5));
  CHECK(recs.retract("/a.pdf//page:1", "gallery floor"));
  CHECK(!recs.retract("/a.pdf//page:1", "gallery floor"));
  CHECK(recs.find("/a.pdf//page:1")->decision("gallery floor") == nullptr);
  recs.forget("/a.pdf//page:1");
  CHECK(recs.find("/a.pdf//page:1") == nullptr);
  CHECK(recs.ensure("/b.png", thumtoo::lod::SourceKind::Image).profile_state
        == thumtoo::lod::ProfileState::NotApplicable);
  recs.clear();
}

/// The Status panel's rows carry the facts: profile verdict, decisions with
/// reasons, tile phase, decode counts, and problems are toned.
void test_status_sections()
{
  thumtoo::lod::SourceStatus st;
  st.record.path = "/doc.pdf//page:3";
  st.record.kind = thumtoo::lod::SourceKind::PdfPage;
  st.record.document_file = "/doc.pdf";
  st.record.page = 3;
  st.record.profile_state = thumtoo::lod::ProfileState::Known;
  st.record.profile = raster_profile(300, -1);
  st.record.profile->images.push_back({2550, 3300, 1, 8, false, 300.0, 1.0});
  st.record.decisions.push_back({"zoom floor", "scale -1", "raster page", 0});
  thumtoo::lod::TileSession::DebugSnapshot t;
  t.phase = thumtoo::lod::TileSession::Phase::Error;
  t.cov.visible = 4;
  t.cov.holes = 2;
  st.tiles = t;
  st.tile_error = "s=-1 0,0: boom";
  thumtoo::PdfDocumentRenderStats rs;
  rs.opens = 1;
  thumtoo::PdfPageRenderStats ps;
  ps.page = 3;
  ps.cells_rendered = 12;
  ps.decode.decodes = 1;
  ps.decode.full_decodes = 1;
  rs.pages.push_back(ps);
  rs.decode = ps.decode;
  st.render = rs;

  auto const sections = thumtoo::lod::build_status_sections(st);
  std::string const text = thumtoo::lod::format_status_text(sections);
  CHECK(text.find("Page analysis") != std::string::npos);
  CHECK(text.find("raster: test") != std::string::npos);
  CHECK(text.find("2550×3300 gray 8-bit · 300 dpi") != std::string::npos);
  CHECK(text.find("zoom floor: scale -1 — raster page") != std::string::npos);
  CHECK(text.find("state: error  [problem]") != std::string::npos);
  CHECK(text.find("holes") != std::string::npos);
  CHECK(text.find("s=-1 0,0: boom") != std::string::npos);
  CHECK(text.find("image decodes: 1 (whole image 1, per-cell subarea 0) for 12 cells —")
        != std::string::npos);
}

int main()
{
  test_dim_at_tile_scale();
  test_negative_scale_content_rect();
  test_planner_negative_scale_stride();
  test_density_allows_negative_min();
  test_max_scale_and_grid();
  test_planner_1to1();
  test_planner_zoomed_out();
  test_planner_partial_viewport();
  test_parent_uv_edge_tile();
  test_edge_tile_content_rect();
  test_content_coverage_odd_widths();
  test_parent_key();

  test_fallback_parent_uv();
  test_lqip_until_tile();
  test_non_ready_exact_falls_to_parent();
  test_empty_plan_without_lqip();

  test_coarse_first_issue_order();
  test_two_views_share_one_request_per_key();
  test_destroying_a_view_never_cancels_peer_demand();
  test_pan_cancels_undemanded_and_reissues();
  test_cancelled_while_demanded_is_reissued();
  test_failed_backoff_then_error_with_reason();
  test_unavailable_is_never_retried();
  test_stall_watchdog_reports_and_recovers();
  test_invalidate_drops_late_results();
  test_content_size_change_drops_cells();
  test_demand_lease_expires_without_renew();
  test_priority_across_paths();
  test_draw_plan_follows_loader_changes();
  test_ok_without_pixels_is_a_failure();
  test_no_busy_wake_when_capacity_full();
  test_document_denser_demand_levels();
  test_shared_key_survives_one_view_leaving();
  test_min_scale_change_replans_static_view();
  test_zoom_floor_policy();
  test_source_record_decisions();
  test_status_sections();

  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "test_lod: all passed\n";
  return 0;
}
