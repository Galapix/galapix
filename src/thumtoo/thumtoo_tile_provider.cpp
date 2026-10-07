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

#include "math/size.hpp"

#include <thumtoo/client.hpp>
#include <thumtoo/pdf.hpp>
#include <thumtoo/epub.hpp>
#include <thumtoo/lod/client_tile_backend.hpp>
#include <thumtoo/constants.hpp>
#include <thumtoo/uri.hpp>

namespace galapix {

namespace {

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
  // Same as thumtoo dim_at_tile_scale / vips_shrink floor-half (not ceil).
  while (w > 256 || h > 256) {
    w = w / 2;
    h = h / 2;
    if (w < 1) w = 1;
    if (h < 1) h = 1;
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
  // Vector/Mixed PDF pages: deep live region tiles (scale down to -8).
  // Raster (scanned) pages: thumtoo answers scales finer than the native
  // image dpi with Unavailable (PdfPageProfile::finest_useful_scale), do
  // not request them.
  constexpr int kPdfMinLiveTileScaleVector = -8;  // 144 * 256 ≈ 36864 dpi
  int min_scale = 0;
  if (thumtoo::is_pdf_page_uri(uri)) {
    min_scale = kPdfMinLiveTileScaleVector;
    if (auto parsed = thumtoo::parse_pdf_uri(uri)) {
      auto const profile = thumtoo::pdf_page_profile(parsed->pdf_path, parsed->page);
      if (profile && profile->finest_useful_scale) {
        min_scale = std::max(min_scale, *profile->finest_useful_scale);
        log_info("ThumtooTileProvider: raster PDF page ({}) min_scale={} {}",
                 thumtoo::pdf_backend_name(parsed->backend), min_scale, uri);
      } else {
        log_debug("ThumtooTileProvider: PDF backend={} min_scale={}",
                  thumtoo::pdf_backend_name(parsed->backend), min_scale);
      }
    } else if (thumtoo::parse_epub_uri(uri)) {
      // Reflowed EPUB pages are vector-like; allow deep live region tiles.
      log_debug("ThumtooTileProvider: EPUB min_scale={}", min_scale);
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
  client->request_size(uri, [&](std::string, thumtoo::SizeReply) {
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

std::shared_ptr<thumtoo::lod::TileBackend>
ThumtooTileProvider::create_backend()
{
  // request_tile_cells / cancel_tile_cells, cells arrive decoded to RGBA8
  // on the Client worker (thumtoo docs/TILE_LOD.md).
  return std::make_shared<thumtoo::lod::ClientTileBackend>(m_client, m_uri);
}

} // namespace galapix

/* EOF */
