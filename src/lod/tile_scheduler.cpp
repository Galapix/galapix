// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/lod/tile_scheduler.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>

namespace thumtoo::lod {

namespace {

bool trace_env()
{
  static bool const on = [] {
    char const* e = std::getenv("THUMTOO_LOD_DEBUG");
    return e && e[0] && e[0] != '0';
  }();
  return on;
}

}  // namespace

TileScheduler& TileScheduler::instance()
{
  static TileScheduler s;
  return s;
}

void TileScheduler::set_wake_hook(std::function<void(std::int64_t)> hook)
{
  std::lock_guard<std::mutex> lock(m_mu);
  m_hook = std::move(hook);
}

void TileScheduler::set_clock(std::function<std::int64_t()> clock)
{
  std::lock_guard<std::mutex> lock(m_mu);
  m_clock = std::move(clock);
}

std::int64_t TileScheduler::now_ms() const
{
  std::function<std::int64_t()> clock;
  {
    std::lock_guard<std::mutex> lock(m_mu);
    clock = m_clock;
  }
  if (clock) {
    return clock();
  }
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch())
      .count();
}

void TileScheduler::register_loader(std::shared_ptr<TileLoader> const& loader)
{
  if (!loader) {
    return;
  }
  loader->set_demand_settle_ms(m_cfg.demand_settle_ms);
  loader->set_clock([this]() { return now_ms(); });
  loader->set_wake([this](std::int64_t delay_ms) { request_pump(delay_ms); });
  std::lock_guard<std::mutex> lock(m_mu);
  m_loaders.push_back(loader);
}

void TileScheduler::request_pump(std::int64_t delay_ms)
{
  std::function<void(std::int64_t)> hook;
  {
    std::lock_guard<std::mutex> lock(m_mu);
    hook = m_hook;
  }
  if (hook) {
    hook(std::max<std::int64_t>(0, delay_ms));
  }
}

std::vector<std::shared_ptr<TileLoader>> TileScheduler::live_loaders()
{
  std::vector<std::shared_ptr<TileLoader>> out;
  std::lock_guard<std::mutex> lock(m_mu);
  out.reserve(m_loaders.size());
  auto it = m_loaders.begin();
  while (it != m_loaders.end()) {
    if (auto sp = it->lock()) {
      out.push_back(std::move(sp));
      ++it;
    } else {
      it = m_loaders.erase(it);
    }
  }
  return out;
}

void TileScheduler::pump()
{
  std::int64_t const now = now_ms();
  std::vector<std::shared_ptr<TileLoader>> loaders = live_loaders();
  ++m_stats.pumps;

  int total_queued = 0;
  int applied = 0;
  for (auto const& l : loaders) {
    applied += l->apply_results(now);
    l->maintain(now);
    total_queued += l->queued_count();
  }

  // Global priority order across paths.
  struct Cand {
    TileLoader* loader = nullptr;
    TileLoader::Issuable cell;
  };
  std::vector<Cand> cands;
  {
    std::vector<TileLoader::Issuable> tmp;
    for (auto const& l : loaders) {
      tmp.clear();
      l->collect_issuable(now, tmp);
      for (auto const& c : tmp) {
        cands.push_back({l.get(), c});
      }
    }
  }
  std::stable_sort(cands.begin(), cands.end(), [](Cand const& a, Cand const& b) {
    return a.cell.priority > b.cell.priority;
  });

  int capacity = m_cfg.max_queued_global - total_queued;
  std::unordered_map<TileLoader*, std::vector<TileKey>> picked;
  std::unordered_map<TileLoader*, int> per_loader;
  for (auto const& l : loaders) {
    per_loader[l.get()] = l->queued_count();
  }
  int waiting = 0;
  for (Cand const& c : cands) {
    if (capacity <= 0) {
      ++waiting;
      continue;
    }
    int& n = per_loader[c.loader];
    if (n >= m_cfg.max_queued_per_loader) {
      ++waiting;
      continue;
    }
    picked[c.loader].push_back(c.cell.key);
    ++n;
    --capacity;
  }

  // Issue in priority order, chunked: each chunk is one backend job so the
  // worker pool runs chunks in parallel and coarse cells (first) land first.
  int const chunk = std::max(1, m_cfg.chunk);
  for (auto const& l : loaders) {
    auto it = picked.find(l.get());
    if (it == picked.end()) {
      continue;
    }
    std::vector<TileKey> const& keys = it->second;
    for (std::size_t i = 0; i < keys.size(); i += static_cast<std::size_t>(chunk)) {
      std::size_t const end =
          std::min(keys.size(), i + static_cast<std::size_t>(chunk));
      l->issue(std::vector<TileKey>(keys.begin() + static_cast<std::ptrdiff_t>(i),
                                    keys.begin() + static_cast<std::ptrdiff_t>(end)),
               now);
      m_stats.issued_total += end - i;
    }
  }

  // Next wake: earliest loader deadline (retry / stall / lease). Results and
  // demand changes wake us on their own; capacity frees up via results.
  std::int64_t next = TileLoader::kNever;
  int queued_after = 0;
  for (auto const& l : loaders) {
    // A deadline already due (retry waiting for capacity) is not a reason to
    // spin: freed capacity always comes with a result, which wakes us.
    std::int64_t const d = l->next_deadline_ms();
    if (d > now) {
      next = std::min(next, d);
    }
    queued_after += l->queued_count();
    if (l->has_pending_results()) {
      next = std::min(next, now);
    }
  }
  m_stats.loaders = static_cast<int>(loaders.size());
  m_stats.queued = queued_after;
  m_stats.issuable = waiting;
  int issued_now = 0;
  for (auto const& [l, keys] : picked) {
    (void)l;
    issued_now += static_cast<int>(keys.size());
  }
  if ((m_cfg.trace || trace_env()) && (applied > 0 || issued_now > 0)) {
    std::fprintf(stderr,
                 "thumtoo/lod-sched: t=%lld applied=%d issued=%d queued=%d "
                 "waiting=%d loaders=%d next=%lld\n",
                 static_cast<long long>(now), applied, issued_now, queued_after,
                 waiting, static_cast<int>(loaders.size()),
                 next == TileLoader::kNever ? -1LL
                                            : static_cast<long long>(next - now));
  }
  if (next != TileLoader::kNever) {
    request_pump(next > now ? next - now : 0);
  }
}

TileScheduler::Stats TileScheduler::stats() const
{
  return m_stats;
}

}  // namespace thumtoo::lod
