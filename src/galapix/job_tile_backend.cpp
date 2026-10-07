// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "galapix/job_tile_backend.hpp"

#include <atomic>
#include <cstring>
#include <map>
#include <mutex>
#include <vector>

#include <surf/pixel_format.hpp>
#include <surf/software_surface.hpp>

#include "job/job_manager.hpp"

namespace galapix {

namespace lod = thumtoo::lod;

/** Outstanding cells, shared with the job callbacks, which can outlive
    the backend */
struct JobTileBackend::Shared
{
  struct Pending
  {
    lod::FetchRequest request{};
    JobHandle handle = JobHandle::create();
    std::atomic<bool> done{false};
  };

  std::mutex mutex{};
  std::multimap<lod::TileKey, std::shared_ptr<Pending>> pending{};

  void forget(std::shared_ptr<Pending> const& p)
  {
    std::lock_guard<std::mutex> lock(mutex);
    auto range = pending.equal_range(p->request.key);
    for (auto it = range.first; it != range.second; ++it) {
      if (it->second == p) {
        pending.erase(it);
        break;
      }
    }
  }
};

lod::TileBitmap
tile_bitmap_from_surface(surf::SoftwareSurface const& surface)
{
  surf::SoftwareSurface const rgba = surface.get_format() == surf::PixelFormat::RGBA8
    ? surface
    : surf::convert(surface, surf::PixelFormat::RGBA8);

  lod::TileBitmap bitmap;
  bitmap.width = rgba.get_width();
  bitmap.height = rgba.get_height();
  bitmap.codec = "rgba8";
  size_t const row_bytes = static_cast<size_t>(bitmap.width) * 4;
  bitmap.bytes.resize(row_bytes * static_cast<size_t>(bitmap.height));
  for (int y = 0; y < bitmap.height; ++y) {
    std::memcpy(bitmap.bytes.data() + row_bytes * static_cast<size_t>(y),
                rgba.get_row_data(y), row_bytes);
  }
  return bitmap;
}

JobTileBackend::JobTileBackend(JobManager& job_manager, int min_scale, int max_scale,
                               MakeJob make_job) :
  m_job_manager(job_manager),
  m_min_scale(min_scale),
  m_max_scale(max_scale),
  m_make_job(std::move(make_job)),
  m_shared(std::make_shared<Shared>())
{
}

JobTileBackend::~JobTileBackend()
{
}

void
JobTileBackend::fetch(std::vector<lod::FetchRequest> cells, ResultFn on_result)
{
  for (lod::FetchRequest const& request : cells)
  {
    if (request.key.scale < m_min_scale || request.key.scale > m_max_scale ||
        request.key.x < 0 || request.key.y < 0)
    {
      on_result(lod::FetchResult{request.key, request.ticket, lod::FetchStatus::Unavailable, {},
                                 "scale outside the provider's range"});
      continue;
    }

    auto pending = std::make_shared<Shared::Pending>();
    pending->request = request;
    {
      std::lock_guard<std::mutex> lock(m_shared->mutex);
      m_shared->pending.emplace(request.key, pending);
    }

    std::weak_ptr<Shared> weak_shared = m_shared;

    auto on_tile = [pending, on_result](Tile tile) {
      if (pending->done.exchange(true)) {
        return;
      }
      on_result(lod::FetchResult{pending->request.key, pending->request.ticket,
                                 lod::FetchStatus::Ok,
                                 tile_bitmap_from_surface(tile.get_surface()), {}});
    };

    auto on_job_done = [pending, on_result, weak_shared](JobPtr const&, bool /*ran*/) {
      if (auto shared = weak_shared.lock()) {
        shared->forget(pending);
      }
      if (pending->done.exchange(true)) {
        return;
      }
      if (pending->handle.is_aborted()) {
        on_result(lod::FetchResult{pending->request.key, pending->request.ticket,
                                   lod::FetchStatus::Cancelled, {}, {}});
      } else {
        on_result(lod::FetchResult{pending->request.key, pending->request.ticket,
                                   lod::FetchStatus::Failed, {}, "tile job produced no tile"});
      }
    };

    m_job_manager.request(m_make_job(pending->handle, request.key.scale,
                                     Vector2i(request.key.x, request.key.y), on_tile),
                          on_job_done);
  }
}

void
JobTileBackend::cancel(std::vector<lod::TileKey> const& keys)
{
  // The job's completion callback reports Cancelled (or the tile, if it
  // was already done).
  std::lock_guard<std::mutex> lock(m_shared->mutex);
  for (lod::TileKey const& key : keys) {
    auto range = m_shared->pending.equal_range(key);
    for (auto it = range.first; it != range.second; ++it) {
      it->second->handle.set_aborted();
    }
  }
}

} // namespace galapix

/* EOF */
