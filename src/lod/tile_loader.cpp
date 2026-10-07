// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/lod/tile_loader.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>

namespace thumtoo::lod {

namespace {

std::string key_label(TileKey const& k)
{
  char buf[64];
  std::snprintf(buf, sizeof buf, "s=%d %d,%d", k.scale, k.x, k.y);
  return buf;
}

}  // namespace

TileLoader::TileLoader(std::shared_ptr<TileBackend> backend, LoaderConfig cfg)
    : m_backend(std::move(backend))
    , m_cfg(cfg)
    , m_inbox(std::make_shared<Inbox>())
{
}

TileLoader::~TileLoader()
{
  // Results still in flight answer into the dead inbox and are dropped. No
  // backend calls here: see cancel_outstanding().
  m_inbox->alive.store(false);
  std::lock_guard<std::mutex> lock(m_inbox->mu);
  m_inbox->pending.clear();
  m_inbox->wake = nullptr;
}

void TileLoader::cancel_outstanding()
{
  std::vector<TileKey> queued;
  for (auto const& [k, c] : m_cells) {
    if (c.state == CellState::Queued && !m_cancel_sent[k]) {
      m_cancel_sent[k] = true;
      queued.push_back(k);
    }
  }
  if (!queued.empty() && m_backend) {
    m_stats.cancels_sent += queued.size();
    m_backend->cancel(queued);
  }
}

std::int64_t TileLoader::now_ms() const
{
  if (m_clock) {
    return m_clock();
  }
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch())
      .count();
}

bool TileLoader::set_content_size(int width, int height)
{
  if (width <= 0 || height <= 0) {
    return false;
  }
  if (m_content_w == width && m_content_h == height) {
    return false;
  }
  if (m_content_w <= 0 || m_content_h <= 0) {
    m_content_w = width;
    m_content_h = height;
    return false;
  }
  char note[96];
  std::snprintf(note, sizeof note, "%dx%d -> %dx%d", m_content_w, m_content_h,
                width, height);
  m_stats.content_change_note = note;
  ++m_stats.content_changes;
  m_content_w = width;
  m_content_h = height;
  // Cells were cut for the old grid: painting them with the new geometry
  // misplaces every cell.
  drop_all_cells_and_cancel();
  notify_views();
  return true;
}

TileLoader::ViewId TileLoader::add_view(std::function<void()> on_change)
{
  ViewId const id = m_next_view++;
  m_views[id].on_change = std::move(on_change);
  return id;
}

void TileLoader::remove_view(ViewId id)
{
  auto it = m_views.find(id);
  if (it == m_views.end()) {
    return;
  }
  bool const had_demand = !it->second.demand.empty();
  m_views.erase(it);
  if (had_demand) {
    m_union_dirty = true;
    request_wake(m_demand_settle_ms);
  }
}

void TileLoader::set_demand(ViewId id, std::vector<Demand> demand,
                            std::int64_t now_ms)
{
  auto it = m_views.find(id);
  if (it == m_views.end()) {
    return;
  }
  it->second.demand = std::move(demand);
  it->second.lease_until_ms = now_ms + m_cfg.demand_lease_ms;
  m_union_dirty = true;
  request_wake(m_demand_settle_ms);
}

void TileLoader::renew_demand(ViewId id, std::int64_t now_ms)
{
  auto it = m_views.find(id);
  if (it == m_views.end() || it->second.demand.empty()) {
    return;
  }
  it->second.lease_until_ms = now_ms + m_cfg.demand_lease_ms;
}

void TileLoader::clear_demand(ViewId id)
{
  auto it = m_views.find(id);
  if (it == m_views.end() || it->second.demand.empty()) {
    return;
  }
  it->second.demand.clear();
  m_union_dirty = true;
  request_wake(m_demand_settle_ms);
}

bool TileLoader::has_demand(ViewId id) const
{
  auto it = m_views.find(id);
  return it != m_views.end() && !it->second.demand.empty();
}

bool TileLoader::is_demanded(TileKey const& key) const
{
  const_cast<TileLoader*>(this)->rebuild_union();
  return m_union.find(key) != m_union.end();
}

void TileLoader::set_wake(std::function<void(std::int64_t)> wake)
{
  std::lock_guard<std::mutex> lock(m_inbox->mu);
  m_inbox->wake = std::move(wake);
}

void TileLoader::request_wake(std::int64_t delay_ms)
{
  std::function<void(std::int64_t)> wake;
  {
    std::lock_guard<std::mutex> lock(m_inbox->mu);
    wake = m_inbox->wake;
  }
  if (wake) {
    wake(delay_ms);
  }
}

bool TileLoader::has_pending_results() const
{
  std::lock_guard<std::mutex> lock(m_inbox->mu);
  return !m_inbox->pending.empty();
}

void TileLoader::rebuild_union()
{
  if (!m_union_dirty) {
    return;
  }
  m_union.clear();
  for (auto const& [id, v] : m_views) {
    (void)id;
    for (Demand const& d : v.demand) {
      auto [it, inserted] = m_union.emplace(d.key, d.priority);
      if (!inserted && d.priority > it->second) {
        it->second = d.priority;
      }
    }
  }
  m_union_dirty = false;
}

void TileLoader::notify_views()
{
  // Copy first: a callback may re-plan and call set_demand on this loader.
  std::vector<std::function<void()>> cbs;
  cbs.reserve(m_views.size());
  for (auto const& [id, v] : m_views) {
    (void)id;
    if (v.on_change) {
      cbs.push_back(v.on_change);
    }
  }
  for (auto const& cb : cbs) {
    cb();
  }
}

void TileLoader::erase_cell(std::map<TileKey, TileCell>::iterator it)
{
  TileCell const& c = it->second;
  if (c.state == CellState::Queued && m_queued > 0) {
    --m_queued;
  }
  if (c.ready()) {
    --m_ready_count;
    m_ready_bytes -= std::min(m_ready_bytes, c.bitmap.bytes.size());
  }
  m_cancel_sent.erase(it->first);
  m_cells.erase(it);
}

void TileLoader::set_cell_ready(TileKey const& key, TileBitmap bitmap,
                                std::int64_t now_ms)
{
  TileCell& c = m_cells[key];
  if (c.state == CellState::Queued && m_queued > 0) {
    --m_queued;
  }
  if (c.ready()) {
    --m_ready_count;
    m_ready_bytes -= std::min(m_ready_bytes, c.bitmap.bytes.size());
  }
  c.state = CellState::Ready;
  c.bitmap = std::move(bitmap);
  c.ticket = 0;
  c.attempts = 0;
  c.error.clear();
  c.since_ms = now_ms;
  c.retry_at_ms = 0;
  c.last_used = ++m_lru_clock;
  if (c.ready()) {
    ++m_ready_count;
    m_ready_bytes += c.bitmap.bytes.size();
  }
  m_cancel_sent.erase(key);
}

void TileLoader::fail_cell(TileCell& cell, TileKey const& key, std::string error,
                           std::int64_t now_ms)
{
  if (cell.state == CellState::Queued && m_queued > 0) {
    --m_queued;
  }
  cell.state = CellState::Failed;
  cell.ticket = 0;
  ++cell.attempts;
  cell.error = std::move(error);
  cell.since_ms = now_ms;
  if (cell.attempts < m_cfg.max_attempts) {
    std::int64_t backoff = m_cfg.retry_base_ms;
    for (int i = 1; i < cell.attempts; ++i) {
      backoff *= 2;
    }
    cell.retry_at_ms = now_ms + backoff;
  } else {
    cell.retry_at_ms = now_ms + m_cfg.failed_cooldown_ms;
  }
  m_stats.last_error = key_label(key) + ": " + cell.error;
  m_cancel_sent.erase(key);
}

int TileLoader::apply_results(std::int64_t now_ms)
{
  std::vector<std::pair<std::uint64_t, FetchResult>> batch;
  {
    std::lock_guard<std::mutex> lock(m_inbox->mu);
    batch.swap(m_inbox->pending);
  }
  if (batch.empty()) {
    return 0;
  }
  bool changed = false;
  int applied = 0;
  for (auto& [epoch, r] : batch) {
    ++m_stats.results_total;
    if (epoch != m_epoch) {
      // Issued for content that was invalidated since.
      ++m_stats.stale_results;
      continue;
    }
    auto it = m_cells.find(r.key);
    if (r.status == FetchStatus::Ok) {
      if (!r.bitmap.valid()) {
        // Contract violation: Ok without pixels. Treat as a failure with a
        // reason instead of a silent hole.
        if (it != m_cells.end() && it->second.state == CellState::Queued
            && it->second.ticket == r.ticket) {
          fail_cell(it->second, r.key, "backend returned Ok without pixels",
                    now_ms);
          changed = true;
          ++applied;
        } else {
          ++m_stats.stale_results;
        }
        continue;
      }
      if (it != m_cells.end() && it->second.ready()) {
        ++m_stats.stale_results;  // duplicate (stall re-request)
        continue;
      }
      // Pixels for this content are valid whatever the ticket: accept even a
      // late reply after a stall re-request.
      set_cell_ready(r.key, std::move(r.bitmap), now_ms);
      changed = true;
      ++applied;
      continue;
    }
    // Non-Ok results only apply to the outstanding request they answer.
    if (it == m_cells.end() || it->second.state != CellState::Queued
        || it->second.ticket != r.ticket) {
      ++m_stats.stale_results;
      continue;
    }
    switch (r.status) {
    case FetchStatus::Cancelled:
      erase_cell(it);
      break;
    case FetchStatus::Failed:
      fail_cell(it->second, r.key,
                r.error.empty() ? std::string("backend failure (no reason given)")
                                : r.error,
                now_ms);
      break;
    case FetchStatus::Unavailable: {
      TileCell& c = it->second;
      --m_queued;
      c.state = CellState::Unavailable;
      c.ticket = 0;
      c.error = r.error.empty() ? std::string("cell unavailable") : r.error;
      c.since_ms = now_ms;
      m_stats.last_error = key_label(r.key) + ": " + c.error;
      m_cancel_sent.erase(r.key);
      break;
    }
    case FetchStatus::Ok:
      break;
    }
    changed = true;
    ++applied;
  }
  if (changed) {
    trim_budget();
    notify_views();
  }
  return applied;
}

void TileLoader::maintain(std::int64_t now_ms)
{
  bool changed = false;

  // 1. Demand leases.
  for (auto& [id, v] : m_views) {
    (void)id;
    if (!v.demand.empty() && v.lease_until_ms <= now_ms) {
      v.demand.clear();
      m_union_dirty = true;
    }
  }
  rebuild_union();

  // 2. Stall watchdog + cancel of undemanded Queued cells.
  std::vector<TileKey> cancel;
  for (auto& [k, c] : m_cells) {
    if (c.state != CellState::Queued) {
      continue;
    }
    if (now_ms - c.since_ms >= m_cfg.stall_timeout_ms) {
      ++m_stats.stalls;
      char msg[128];
      std::snprintf(msg, sizeof msg,
                    "no reply from tile backend after %lld ms "
                    "(backend contract violation)",
                    static_cast<long long>(now_ms - c.since_ms));
      std::fprintf(stderr, "thumtoo/lod: STALL %s %s\n", key_label(k).c_str(),
                   msg);
      fail_cell(c, k, msg, now_ms);
      changed = true;
      continue;
    }
    if (m_union.find(k) == m_union.end() && !m_cancel_sent[k]) {
      m_cancel_sent[k] = true;
      cancel.push_back(k);
    }
  }
  if (!cancel.empty() && m_backend) {
    m_stats.cancels_sent += cancel.size();
    m_backend->cancel(cancel);
  }

  // 3. Budget.
  std::size_t const before = m_ready_count;
  trim_budget();
  if (m_ready_count != before) {
    changed = true;
  }

  if (changed) {
    notify_views();
  }
}

void TileLoader::trim_budget()
{
  if (m_ready_bytes <= m_cfg.byte_budget) {
    return;
  }
  rebuild_union();
  std::vector<std::pair<std::uint64_t, TileKey>> victims;
  for (auto const& [k, c] : m_cells) {
    if (c.ready() && m_union.find(k) == m_union.end()) {
      victims.emplace_back(c.last_used, k);
    }
  }
  std::sort(victims.begin(), victims.end(),
            [](auto const& a, auto const& b) { return a.first < b.first; });
  for (auto const& [lu, k] : victims) {
    (void)lu;
    if (m_ready_bytes <= m_cfg.byte_budget) {
      break;
    }
    auto it = m_cells.find(k);
    if (it != m_cells.end()) {
      erase_cell(it);
      ++m_stats.evicted;
    }
  }
}

void TileLoader::collect_issuable(std::int64_t now_ms,
                                  std::vector<Issuable>& out) const
{
  const_cast<TileLoader*>(this)->rebuild_union();
  if (m_content_w <= 0 || m_content_h <= 0) {
    return;
  }
  for (auto const& [k, prio] : m_union) {
    auto it = m_cells.find(k);
    if (it == m_cells.end()) {
      out.push_back({k, prio});
    } else if (it->second.state == CellState::Failed
               && it->second.retry_at_ms <= now_ms) {
      out.push_back({k, prio});
    }
  }
}

void TileLoader::issue(std::vector<TileKey> const& keys, std::int64_t now_ms)
{
  if (keys.empty() || !m_backend) {
    return;
  }
  std::vector<FetchRequest> reqs;
  reqs.reserve(keys.size());
  for (TileKey const& k : keys) {
    if (auto it = m_cells.find(k); it != m_cells.end()) {
      CellState const st = it->second.state;
      // Only Failed cells are re-issued: Queued is already outstanding (never
      // duplicate), Ready is done, Unavailable is permanent.
      if (st != CellState::Failed) {
        continue;
      }
    }
    TileCell& c = m_cells[k];
    c.state = CellState::Queued;
    c.bitmap = {};
    c.ticket = m_next_ticket++;
    c.since_ms = now_ms;
    c.retry_at_ms = 0;
    ++m_queued;
    m_cancel_sent.erase(k);
    reqs.push_back({k, c.ticket});
  }
  if (reqs.empty()) {
    return;
  }
  m_stats.issued_total += reqs.size();
  std::shared_ptr<Inbox> inbox = m_inbox;
  std::uint64_t const epoch = m_epoch;
  m_backend->fetch(std::move(reqs), [inbox, epoch](FetchResult r) {
    if (!inbox->alive.load()) {
      return;
    }
    std::function<void(std::int64_t)> wake;
    {
      std::lock_guard<std::mutex> lock(inbox->mu);
      if (!inbox->alive.load()) {
        return;
      }
      inbox->pending.emplace_back(epoch, std::move(r));
      wake = inbox->wake;
    }
    if (wake) {
      wake(0);
    }
  });
}

std::int64_t TileLoader::next_deadline_ms() const
{
  const_cast<TileLoader*>(this)->rebuild_union();
  std::int64_t t = kNever;
  for (auto const& [id, v] : m_views) {
    (void)id;
    if (!v.demand.empty()) {
      t = std::min(t, v.lease_until_ms);
    }
  }
  for (auto const& [k, c] : m_cells) {
    if (c.state == CellState::Queued) {
      t = std::min(t, c.since_ms + m_cfg.stall_timeout_ms);
    } else if (c.state == CellState::Failed
               && m_union.find(k) != m_union.end()) {
      t = std::min(t, c.retry_at_ms);
    }
  }
  return t;
}

void TileLoader::drop_all_cells_and_cancel()
{
  std::vector<TileKey> queued;
  for (auto const& [k, c] : m_cells) {
    if (c.state == CellState::Queued) {
      queued.push_back(k);
    }
  }
  if (!queued.empty() && m_backend) {
    m_stats.cancels_sent += queued.size();
    m_backend->cancel(queued);
  }
  m_cells.clear();
  m_cancel_sent.clear();
  m_queued = 0;
  m_ready_count = 0;
  m_ready_bytes = 0;
  // Results already in flight belong to the old content.
  ++m_epoch;
  {
    std::lock_guard<std::mutex> lock(m_inbox->mu);
    m_inbox->pending.clear();
  }
}

void TileLoader::invalidate(std::string const& why)
{
  (void)why;
  drop_all_cells_and_cancel();
  m_stats.last_error.clear();
  notify_views();
  request_wake(0);
}

TileCell const* TileLoader::find(TileKey const& key) const
{
  auto it = m_cells.find(key);
  return it == m_cells.end() ? nullptr : &it->second;
}

bool TileLoader::has_ready_at_scale(int scale) const
{
  for (auto const& [k, c] : m_cells) {
    if (k.scale == scale && c.ready()) {
      return true;
    }
  }
  return false;
}

void TileLoader::touch(TileKey const& key)
{
  auto it = m_cells.find(key);
  if (it != m_cells.end() && it->second.ready()) {
    it->second.last_used = ++m_lru_clock;
  }
}

TileLoader::Stats TileLoader::stats() const
{
  Stats s = m_stats;
  for (auto const& [k, c] : m_cells) {
    (void)k;
    switch (c.state) {
    case CellState::Queued:
      ++s.queued;
      break;
    case CellState::Ready:
      ++s.ready;
      break;
    case CellState::Failed:
      ++s.failed;
      break;
    case CellState::Unavailable:
      ++s.unavailable;
      break;
    }
  }
  return s;
}

}  // namespace thumtoo::lod
