// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "galapix/tile_lod.hpp"

#include <atomic>
#include <limits>
#include <map>
#include <mutex>
#include <string>

#include <thumtoo/lod/tile_backend.hpp>

#include "galapix/viewer.hpp"

namespace galapix::tile_lod {

namespace lod = thumtoo::lod;

namespace {

constexpr std::int64_t kNever = std::numeric_limits<std::int64_t>::max();

/** Scheduler time at which pump() is due, kNever when nothing is pending */
std::atomic<std::int64_t> g_pump_at{kNever};

void
install_scheduler()
{
  static std::once_flag once;
  std::call_once(once, [] {
    lod::TileScheduler& scheduler = lod::TileScheduler::instance();
    scheduler.set_wake_hook([](std::int64_t delay_ms) {
      // any thread
      std::int64_t const at = lod::TileScheduler::instance().now_ms() + delay_ms;
      std::int64_t cur = g_pump_at.load();
      while (at < cur && !g_pump_at.compare_exchange_weak(cur, at)) {}
      if (Viewer* viewer = Viewer::current()) {
        viewer->redraw();
      }
    });
  });
}

lod::LoaderConfig
loader_config()
{
  lod::LoaderConfig cfg;
  // Galapix shows many images at once, keep each loader's RGBA8 cells
  // small (biltoo: 512 MiB for its few paths). Only undemanded cells are
  // evicted.
  cfg.byte_budget = 64ull * 1024ull * 1024ull;
  return cfg;
}

} // namespace

std::shared_ptr<lod::TileLoader>
loader_for(TileProvider& provider)
{
  install_scheduler();

  static std::mutex mutex;
  static std::map<std::string, std::weak_ptr<lod::TileLoader>> loaders;

  std::string const key = provider.get_loader_key();

  std::lock_guard<std::mutex> lock(mutex);
  if (!key.empty()) {
    if (auto it = loaders.find(key); it != loaders.end()) {
      if (auto loader = it->second.lock()) {
        return loader;
      }
    }
  }

  auto loader = std::make_shared<lod::TileLoader>(provider.create_backend(), loader_config());
  lod::TileScheduler::instance().register_loader(loader);

  if (!key.empty()) {
    for (auto it = loaders.begin(); it != loaders.end();) {
      it = it->second.expired() ? loaders.erase(it) : std::next(it);
    }
    loaders[key] = loader;
  }
  return loader;
}

void
pump_if_due()
{
  lod::TileScheduler& scheduler = lod::TileScheduler::instance();
  std::int64_t at = g_pump_at.load();
  if (at == kNever || scheduler.now_ms() < at) {
    return;
  }
  // A wake recorded meanwhile with an earlier time stays armed. Later
  // ones folded into this pump are fine: pump() applies every result and
  // re-arms the next deadline through the wake hook.
  g_pump_at.compare_exchange_strong(at, kNever);
  scheduler.pump();
}

lod::TileScheduler::Stats
scheduler_stats()
{
  return lod::TileScheduler::instance().stats();
}

} // namespace galapix::tile_lod

/* EOF */
