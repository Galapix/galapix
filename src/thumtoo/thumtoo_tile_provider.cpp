// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "thumtoo/thumtoo_tile_provider.hpp"

#include "thumtoo/thumtoo_callback_queue.hpp"

#include <algorithm>
#include <optional>
#include <logmich/log.hpp>
#include <surf/convert.hpp>
#include <surf/pixel_format.hpp>
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

/** Decode JPEG tile bytes. Returns nullopt on empty payload or decode error.
 *
 *  Converts to RGBA8 so row pitch is width*4 (multiple of 4). wstdisplay
 *  Texture upload uses GL_UNPACK_ALIGNMENT=4; tightly packed RGB8 rows with
 *  width not divisible by 4 (common for edge tiles) otherwise shear and look
 *  like scrambled greyscale.
 */
std::optional<surf::SoftwareSurface> surface_from_tile_blob(thumtoo::TileBlob const& tb)
{
  if (tb.bytes.empty()) {
    return std::nullopt;
  }
  try {
    auto surface = surf::jpeg::load_from_mem(
      std::span<uint8_t const>(tb.bytes.data(), tb.bytes.size()));
    if (surface.get_format() != surf::PixelFormat::RGBA8) {
      surface = surf::convert(surface, surf::PixelFormat::RGBA8);
    }
    return surface;
  } catch (std::exception const& err) {
    log_error("ThumtooTileProvider: JPEG decode failed: {}", err.what());
    return std::nullopt;
  } catch (...) {
    log_error("ThumtooTileProvider: JPEG decode failed");
    return std::nullopt;
  }
}

/** Max scale at which the image still fits in a single kTileSize (256) tile.
 *  Must match thumtoo::Client::get_tile_coverage theoretical range and
 *  cut_pyramid_from_vips — NOT Galapix ImageEntry (which continues to ≤8px).
 *  Requesting higher scales yields empty TileBlob (min_scale > computed_max).
 */
int max_scale_for_size(int width, int height)
{
  int w = width;
  int h = height;
  int max_scale = 0;
  // Same loop as thumtoo (TILES.md / get_tile_coverage).
  while (w > 256 || h > 256) {
    w = (w + 1) / 2;
    h = (h + 1) / 2;
    ++max_scale;
  }
  return max_scale;
}

} // namespace

/** Build provider when dimensions are already known (or in thumtoo cache). */
TileProviderPtr
ThumtooTileProvider::create_from_size(std::shared_ptr<thumtoo::Client> client,
                                      std::string uri,
                                      int width, int height)
{
  if (!client || uri.empty() || width <= 0 || height <= 0) {
    return {};
  }

  int max_scale = max_scale_for_size(width, height);
  if (auto cov = client->get_tile_coverage(uri)) {
    max_scale = cov->max_scale;
  }

  log_info("ThumtooTileProvider: {} {}x{} max_scale={}",
           uri, width, height, max_scale);

  return std::make_shared<ThumtooTileProvider>(
    std::move(client), std::move(uri), Size(width, height), max_scale);
}

TileProviderPtr
ThumtooTileProvider::create(std::shared_ptr<thumtoo::Client> client,
                            std::string uri)
{
  if (!client || uri.empty()) {
    return {};
  }

  // Prefer cached size — no I/O.
  if (auto sz = client->get_size(uri)) {
    return create_from_size(std::move(client), std::move(uri),
                            sz->width, sz->height);
  }

  // Single-URI path: one request_size + drain (slow if called in a loop).
  // Bulk open should batch request_size then drain once (ViewerCommand).
  bool done = false;
  client->request_size(uri, [&](std::string, std::optional<thumtoo::Size>) {
    done = true;
  });
  client->drain();
  ThumtooCallbackQueue::instance().pump();
  if (!done) {
    log_error("ThumtooTileProvider: size probe did not complete for " + uri);
    return {};
  }

  auto sz = client->get_size(uri);
  if (!sz || sz->width <= 0 || sz->height <= 0) {
    log_error("ThumtooTileProvider: no size for " + uri);
    return {};
  }

  return create_from_size(std::move(client), std::move(uri),
                          sz->width, sz->height);
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

  auto deliver = [job_handle, callback, tilescale, pos, uri = m_uri](
                   std::optional<thumtoo::TileBlob> tb) mutable {
    if (job_handle.is_aborted()) {
      return;
    }
    if (!tb || tb->bytes.empty()) {
      // Common while generating; stand-ins cover the cell until retry succeeds.
      log_debug("ThumtooTileProvider: no tile {} scale={} pos=({},{})",
                uri, tilescale, pos.x(), pos.y());
      job_handle.set_failed();
      return;
    }
    auto surface = surface_from_tile_blob(*tb);
    if (!surface) {
      log_error("ThumtooTileProvider: decode failed {} scale={} pos=({},{})",
                uri, tilescale, pos.x(), pos.y());
      job_handle.set_failed();
      return;
    }
    callback(Tile(tilescale, pos, *surface));
    job_handle.set_finished();
  };

  // Always complete on a Client worker (inline executor): never JPEG-decode on
  // the GUI thread during draw. Durable cache hits are still handled inside
  // Client::request_tile via get_tile (no re-encode).
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
