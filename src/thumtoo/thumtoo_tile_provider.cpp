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
#include <surf/color.hpp>
#include <surf/convert.hpp>
#include <surf/software_surface.hpp>
#include <surf/pixel_format.hpp>
#include <surf/plugins/jpeg.hpp>

#include "job/job_handle.hpp"
#include "math/size.hpp"
#include "math/vector2i.hpp"
#include "util/blob.hpp"

#include <thumtoo/client.hpp>
#include <thumtoo/pdf.hpp>
#include <thumtoo/constants.hpp>
#include <thumtoo/uri.hpp>

namespace galapix {

namespace {

/** Decode tile bytes: rgb888 live cells or JPEG durable cache.
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

  // Live PDF path: raw RGB888 from thumtoo (no JPEG).
  if (tb.codec == "rgb888") {
    int const w = tb.width > 0 ? tb.width : 0;
    int const h = tb.height > 0 ? tb.height : 0;
    if (w <= 0 || h <= 0) {
      return std::nullopt;
    }
    std::size_t const need =
      static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 3u;
    if (tb.bytes.size() < need) {
      log_error("ThumtooTileProvider: rgb888 size mismatch {} < {}",
                tb.bytes.size(), need);
      return std::nullopt;
    }
    auto surface = surf::SoftwareSurface::create(
      surf::PixelFormat::RGBA8, geom::isize(w, h));
    auto& view = surface.as_pixelview<surf::RGBA8Pixel>();
    for (int y = 0; y < h; ++y) {
      std::uint8_t const* src =
        tb.bytes.data() +
        static_cast<std::size_t>(y) * static_cast<std::size_t>(w) * 3u;
      surf::RGBA8Pixel* dst = view.get_row(y);
      for (int x = 0; x < w; ++x) {
        dst[x] = surf::RGBA8Pixel(src[x * 3 + 0], src[x * 3 + 1],
                                  src[x * 3 + 2], 255);
      }
    }
    return surface;
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
  // Coverage max can be negative if only deep PDF cells were ever stored —
  // never let that shrink the theoretical range from native size.
  if (auto cov = client->get_tile_coverage(uri)) {
    max_scale = std::max(max_scale, cov->max_scale);
  }

  // PDF pages can region-render sharper than layout; raster stays at 0.
  // dpi = kPdfLayoutDpi * 2^{-scale} (thumtoo). Live path is O(tile) memory
  // so deep zoom is cheap; the previous −4 floor (~2304 dpi) stopped refining
  // around “one tile ≈ one character”. −8 ≈ 36.8k dpi is well past readable
  // text; thumtoo does not durable-cache below kPdfMinDurableTileScale (−2).
  // Vector PDFs: deep live region tiles (scale down to -8). Image-heavy /
  // scanned pages: layout scale only — region render re-decodes huge JPEG
  // XObjects per cell and is not useful past ~layout dpi.
  constexpr int kPdfMinLiveTileScaleVector = -8;  // 144 * 256 ≈ 36864 dpi
  int min_scale = 0;
  if (thumtoo::is_pdf_page_uri(uri)) {
    min_scale = kPdfMinLiveTileScaleVector;
    if (auto parsed = thumtoo::parse_pdf_uri(uri)) {
      if (!thumtoo::pdf_page_allows_live_tiles(parsed->pdf_path, parsed->page,
                                               parsed->backend)) {
        min_scale = 0;
        log_info("ThumtooTileProvider: image-heavy PDF ({}) min_scale=0 {}",
                 thumtoo::pdf_backend_name(parsed->backend), uri);
      } else {
        log_debug("ThumtooTileProvider: PDF backend={} min_scale={}",
                  thumtoo::pdf_backend_name(parsed->backend), min_scale);
      }
    }
  }

  log_info("ThumtooTileProvider: {} {}x{} scale=[{},{}]",
           uri, width, height, min_scale, max_scale);

  return std::make_shared<ThumtooTileProvider>(
    std::move(client), std::move(uri), Size(width, height), max_scale,
    min_scale);
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
  int max_scale, int min_scale) :
  m_client(std::move(client)),
  m_uri(std::move(uri)),
  m_size(size),
  m_max_scale(max_scale),
  m_min_scale(min_scale)
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
    // cancel_jobs may have erased the cache entry already; still mark the
    // handle so any stale entry is treated as dead and re-queued.
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
      log_error(
        "ThumtooTileProvider: decode failed {} scale={} pos=({},{}) "
        "codec={} bytes={} meta={}x{}",
        uri, tilescale, pos.x(), pos.y(), tb->codec, tb->bytes.size(),
        tb->width, tb->height);
      job_handle.set_failed();
      return;
    }
    log_debug("ThumtooTileProvider: delivered {} scale={} pos=({},{}) "
              "codec={} {}x{}",
              uri, tilescale, pos.x(), pos.y(), tb->codec, tb->width,
              tb->height);
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



void
ThumtooTileProvider::request_tiles(std::vector<TileRequest> requests)
{
  if (requests.empty()) {
    return;
  }

  auto cbs = std::make_shared<std::vector<TileRequest>>(std::move(requests));
  std::vector<thumtoo::Client::TileCoord> coords;
  coords.reserve(cbs->size());
  for (auto const& r : *cbs) {
    coords.push_back(
      thumtoo::Client::TileCoord{r.scale, r.pos.x(), r.pos.y()});
  }

  // Index-based completion: never re-match scale/x/y (missed matches left
  // JobHandles REQUESTED forever and tiles never appeared).
  m_client->request_tiles(
    m_uri, std::move(coords),
    [cbs](std::size_t index, std::optional<thumtoo::TileBlob> tb) {
      if (index >= cbs->size()) {
        return;
      }
      TileRequest& r = (*cbs)[index];
      if (r.job_handle.is_aborted()) {
        return;
      }
      if (!tb || tb->bytes.empty()) {
        r.job_handle.set_failed();
        return;
      }
      auto surface = surface_from_tile_blob(*tb);
      if (!surface) {
        r.job_handle.set_failed();
        return;
      }
      try {
        if (r.callback) {
          r.callback(Tile(r.scale, r.pos, *surface));
        }
        r.job_handle.set_finished();
      } catch (...) {
        r.job_handle.set_failed();
      }
    });
}

} // namespace galapix

/* EOF */
