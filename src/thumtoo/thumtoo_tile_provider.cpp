// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "thumtoo/thumtoo_tile_provider.hpp"

#include <algorithm>
#include <logmich/log.hpp>
#include <surf/plugins/jpeg.hpp>

#include "job/job_handle.hpp"
#include "math/size.hpp"
#include "math/vector2i.hpp"
#include "util/blob.hpp"

#include <thumtoo/client.hpp>
#include <thumtoo/constants.hpp>
#include <thumtoo/uri.hpp>

namespace galapix {

namespace {

surf::SoftwareSurface surface_from_tile_blob(thumtoo::TileBlob const& tb)
{
  if (tb.bytes.empty()) {
    return {};
  }
  Blob blob = Blob::copy(std::span<uint8_t const>(tb.bytes.data(), tb.bytes.size()));
  try {
    return surf::jpeg::load_from_mem(blob);
  } catch (...) {
    log_error("ThumtooTileProvider: JPEG decode failed");
    return {};
  }
}

int max_scale_for_size(int width, int height)
{
  // Match ImageEntry: keep halving until max edge <= 8.
  int s = std::max(width, height);
  int max_scale = 0;
  while (s > 8) {
    s /= 2;
    ++max_scale;
  }
  return max_scale;
}

} // namespace

TileProviderPtr
ThumtooTileProvider::create(std::shared_ptr<thumtoo::Client> client,
                            std::string uri)
{
  if (!client || uri.empty()) {
    return {};
  }

  // Ensure size is known (synchronous probe via request + drain).
  // Default Executor runs callbacks inline from the worker; drain() waits until
  // the queue and in-flight jobs are idle.
  if (!client->get_size(uri)) {
    bool done = false;
    client->request_size(uri, [&](std::string, std::optional<thumtoo::Size>) {
      done = true;
    });
    client->drain();
    if (!done) {
      log_error("ThumtooTileProvider: size probe did not complete for " + uri);
      return {};
    }
  }

  auto sz = client->get_size(uri);
  if (!sz || sz->width <= 0 || sz->height <= 0) {
    log_error("ThumtooTileProvider: no size for " + uri);
    return {};
  }

  int max_scale = max_scale_for_size(sz->width, sz->height);
  if (auto cov = client->get_tile_coverage(uri)) {
    // Prefer stored pyramid range when present.
    max_scale = std::max(max_scale, cov->max_scale);
  }

  log_info("ThumtooTileProvider: {} {}x{} max_scale={}",
           uri, sz->width, sz->height, max_scale);

  return std::make_shared<ThumtooTileProvider>(
    std::move(client), std::move(uri),
    Size(sz->width, sz->height), max_scale);
}

ThumtooTileProvider::ThumtooTileProvider(
  std::shared_ptr<thumtoo::Client> client, std::string uri, Size size,
  int max_scale) :
  m_client(std::move(client)),
  m_uri(std::move(uri)),
  m_size(size),
  m_max_scale(max_scale)
{
}

JobHandle
ThumtooTileProvider::request_tile(int tilescale, Vector2i const& pos,
                                  const std::function<void(Tile)>& callback)
{
  JobHandle job_handle = JobHandle::create();
  const int x = pos.x();
  const int y = pos.y();

  auto deliver = [job_handle, callback, tilescale, pos](std::optional<thumtoo::TileBlob> tb) mutable {
    if (job_handle.is_aborted()) {
      return;
    }
    if (!tb) {
      job_handle.set_failed();
      return;
    }
    auto surface = surface_from_tile_blob(*tb);
    if (!surface) {
      job_handle.set_failed();
      return;
    }
    callback(Tile(tilescale, pos, surface));
    job_handle.set_finished();
  };

  if (auto hit = m_client->get_tile(m_uri, tilescale, x, y)) {
    deliver(std::move(hit));
    return job_handle;
  }

  m_client->request_tile(
    m_uri, tilescale, x, y,
    [deliver = std::move(deliver)](std::string, int, int, int,
                                   std::optional<thumtoo::TileBlob> tb) mutable {
      deliver(std::move(tb));
    });

  return job_handle;
}

} // namespace galapix

/* EOF */
