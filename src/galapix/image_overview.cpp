// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "galapix/image_overview.hpp"

#include <algorithm>

#include <glad/gl.h>

#include <logmich/log.hpp>
#include <surf/convert.hpp>
#include <surf/software_surface_factory.hpp>
#include <surf/pixel_format.hpp>
#include <surf/pixel_data.hpp>
#include <wstdisplay/graphics_context.hpp>
#include <wstdisplay/texture.hpp>

#include "job/job_manager.hpp"
#include "jobs/overview_load_job.hpp"
#include "math/math.hpp"
#include "math/vector2i.hpp"
#include "galapix/app.hpp"
#include "galapix/image_tile_cache.hpp"
#include "galapix/viewer.hpp"
#include "util/weak_functor.hpp"

#ifdef HAVE_THUMTOO
#  include "thumtoo/thumtoo_tile_provider.hpp"
#  include <thumtoo/client.hpp>
#  include <thumtoo/lqip.hpp>
#  include <thumtoo/types.hpp>
#endif

namespace galapix {

namespace {

/** Pick TileGenerator min_scale so JPEG DCT scale yields ~target long edge. */
int min_scale_for_overview(int original_width, int original_height,
                           int target_long_edge = 512)
{
  int const long_edge = std::max(original_width, original_height);
  if (long_edge <= 0) {
    return 3; // jpeg_scale 8
  }
  // jpeg_scale = min(pow2(min_scale), 8)
  for (int min_scale = 0; min_scale <= 3; ++min_scale) {
    int const jpeg_scale = std::min(Math::pow2(min_scale), 8);
    if (long_edge / jpeg_scale <= target_long_edge) {
      return min_scale;
    }
  }
  return 3;
}

wstdisplay::SurfacePtr surface_from_software(surf::SoftwareSurface image)
{
  if (image.get_format() != surf::PixelFormat::RGBA8) {
    image = surf::convert(image, surf::PixelFormat::RGBA8);
  }

  auto texture = wstdisplay::Texture::create(GL_TEXTURE_2D, image.get_size());
  texture->put(image, 0, 0);
  texture->set_filter(GL_LINEAR);

  float const tw = static_cast<float>(texture->get_width());
  float const th = static_cast<float>(texture->get_height());
  float const maxu = static_cast<float>(image.get_width()) / tw;
  float const maxv = static_cast<float>(image.get_height()) / th;

  return wstdisplay::Surface::create(
    texture,
    geom::frect(0.0f, 0.0f, maxu, maxv),
    geom::fsize(static_cast<float>(image.get_width()),
                static_cast<float>(image.get_height())));
}

} // namespace

void
ImageOverview::ensure_requested(JobManager* job_manager, URL const& url,
                                int original_width, int original_height,
                                TileProviderPtr provider,
                                int target_long_edge)
{
  if (m_state == State::Failed) {
    return;
  }

  std::weak_ptr<ImageOverview> weak_self;
  try {
    weak_self = shared_from_this();
  } catch (std::bad_weak_ptr const&) {
    m_state = State::Failed;
    return;
  }

#ifdef HAVE_THUMTOO
  if (auto* tp = dynamic_cast<ThumtooTileProvider*>(provider.get())) {
    // 1) Inline ThumbHash from content row — no blob I/O, allowed in cache-only.
    // Retry every frame until present: size probe writes LQIP asynchronously, so
    // the first ensure_requested often races ahead of get_lqip().
    if (!m_lqip_only && !m_surface) {
      if (auto hash = tp->client()->ensure_lqip(tp->uri())) {
        if (auto img = thumtoo::lqip_decode_rgba(
                std::span<std::uint8_t const>(hash->data(), hash->size()))) {
          try {
            auto surface = surf::SoftwareSurface::create(
              surf::PixelFormat::RGBA8,
              geom::isize(img->width, img->height));
            auto& view = surface.as_pixelview<surf::RGBA8Pixel>();
            for (int y = 0; y < img->height; ++y) {
              std::uint8_t const* src =
                img->rgba.data() +
                static_cast<std::size_t>(y) *
                  static_cast<std::size_t>(img->width) * 4u;
              surf::RGBA8Pixel* dst = view.get_row(y);
              for (int x = 0; x < img->width; ++x) {
                dst[x] = surf::RGBA8Pixel(src[x * 4 + 0], src[x * 4 + 1],
                                          src[x * 4 + 2], src[x * 4 + 3]);
              }
            }
            log_debug("ImageOverview: LQIP decode {}x{} for {}",
                      img->width, img->height, tp->uri());
            m_state = State::Loading;
            receive_software(std::move(surface), /*from_lqip=*/true);
          } catch (std::exception const& err) {
            log_debug("ImageOverview: LQIP surface create failed for {}: {}",
                      tp->uri(), err.what());
          } catch (...) {
            log_debug("ImageOverview: LQIP surface create failed for {}",
                      tp->uri());
          }
        } else {
          log_debug("ImageOverview: ThumbHash decode failed for {} ({} bytes)",
                    tp->uri(), hash->size());
        }
      }
      else if (!m_lqip_miss_logged) {
        // One-shot: common before size probe; if it never appears, content_id
        // is missing or VIPS ThumbHash encode failed.
        m_lqip_miss_logged = true;
        log_debug("ImageOverview: ensure_lqip empty for {} (will retry silently)",
                  tp->uri());
      }
    }

    // Levels upgrade is ensure_levels() — after grid issue_requests (budget).
    return;
  }
#endif

  if (m_state != State::Idle) {
    return;
  }

  // Local files via JobManager + libjpeg DCT scale.
  if (!job_manager || !url.has_stdio_name()) {
    m_state = State::Failed;
    return;
  }

  m_state = State::Loading;
  m_job = JobHandle::create();

  int const edge = std::clamp(target_long_edge > 0 ? target_long_edge : 256, 128, 512);
  int const min_scale = min_scale_for_overview(original_width, original_height, edge);

  auto job = std::make_shared<OverviewLoadJob>(
    m_job, url, min_scale,
    [weak_self](std::optional<surf::SoftwareSurface> surface) {
      if (auto self = weak_self.lock()) {
        self->receive_software(std::move(surface), false);
      }
    });

  job_manager->request(job);
}


void
ImageOverview::ensure_levels(TileProviderPtr provider, int target_long_edge)
{
#ifdef HAVE_THUMTOO
  if (m_state == State::Failed) {
    return;
  }

  auto* tp = dynamic_cast<ThumtooTileProvider*>(provider.get());
  if (!tp) {
    return;
  }

  std::weak_ptr<ImageOverview> weak_self;
  try {
    weak_self = shared_from_this();
  } catch (std::bad_weak_ptr const&) {
    return;
  }

  // Postage-stamp gallery: LQIP alone is enough.
  if (m_lqip_only && m_state == State::Ready && target_long_edge <= 64) {
    return;
  }
  if (m_levels_requested && m_state == State::Loading) {
    return;
  }
  if (m_state == State::Ready && !m_lqip_only) {
    return; // full overview already
  }

  if (!ImageTileCache::tile_requests_enabled()) {
    return;
  }

  // Spend remaining per-frame budget after grid issue_requests.
  if (!ImageTileCache::try_consume_request_budget()) {
    return;
  }
  m_state = State::Loading;
  m_levels_requested = true;
  m_job = JobHandle::create();
  int edge = target_long_edge > 0 ? target_long_edge : 256;
  edge = std::clamp(edge, 128, 512);
  auto client = tp->client();
  std::string uri = tp->uri();
  client->request_pixels(
    std::move(uri), edge,
    [weak_self, job = m_job](std::string, int,
                             std::optional<thumtoo::PixelLevel> px) mutable {
      auto self = weak_self.lock();
      if (!self) {
        return;
      }
      if (job.is_aborted()) {
        return;
      }
      if (!px || px->bytes.empty() || px->width <= 0 || px->height <= 0) {
        if (self->m_lqip_only) {
          self->m_state = State::Ready;
          self->m_levels_requested = false;
          job.set_failed();
        } else {
          self->receive_software(std::nullopt, false);
          job.set_failed();
        }
        return;
      }
      try {
        auto surface = g_app.surface_factory().from_mem(
          std::span<uint8_t const>(px->bytes.data(), px->bytes.size()),
          px->codec == "jxl" ? "image/jxl" : px->codec,
          "thumtoo-level");
        if (surface.get_width() <= 0) {
          if (self->m_lqip_only) {
            self->m_state = State::Ready;
            self->m_levels_requested = false;
            job.set_failed();
          } else {
            self->receive_software(std::nullopt, false);
            job.set_failed();
          }
          return;
        }
        self->receive_software(std::move(surface), /*from_lqip=*/false);
        job.set_finished();
      } catch (...) {
        if (self->m_lqip_only) {
          self->m_state = State::Ready;
          self->m_levels_requested = false;
          job.set_failed();
        } else {
          self->receive_software(std::nullopt, false);
          job.set_failed();
        }
      }
    });
#else
  (void)provider;
  (void)target_long_edge;
#endif
}

void
ImageOverview::receive_software(std::optional<surf::SoftwareSurface> surface,
                                bool from_lqip)
{
  // May run on worker or main: queue is thread-safe; GL upload in process().
  if (from_lqip) {
    m_lqip_only = true;
  } else if (surface.has_value()) {
    m_lqip_only = false;
  }
  m_queue.wait_and_push(std::move(surface));
  if (Viewer* v = Viewer::current()) {
    v->redraw();
  }
}

void
ImageOverview::process()
{
  std::optional<surf::SoftwareSurface> surface;
  if (!m_queue.try_pop(surface)) {
    return;
  }

  if (!surface.has_value() || surface->get_width() <= 0 || surface->get_height() <= 0) {
    // Keep an existing LQIP surface if a levels upgrade failed.
    if (m_surface) {
      m_state = State::Ready;
    } else {
      m_state = State::Failed;
      m_surface.reset();
    }
    return;
  }

  try {
    int const sw = surface->get_width();
    int const sh = surface->get_height();
    m_surface = surface_from_software(std::move(*surface));
    m_state = State::Ready;
    if (m_lqip_only) {
      log_debug("ImageOverview: LQIP GL upload ready {}x{}", sw, sh);
    }
  } catch (std::exception const& err) {
    log_debug("ImageOverview: GL upload failed: {}", err.what());
    m_state = State::Failed;
    m_surface.reset();
  }
}

void
ImageOverview::draw(wstdisplay::GraphicsContext& gc, Rectf const& image_rect)
{
  if (m_surface) {
    m_surface->draw(gc, image_rect);
  }
}

void
ImageOverview::clear()
{
  m_job.set_aborted();
  m_state = State::Idle;
  m_surface.reset();
  m_lqip_only = false;
  m_levels_requested = false;
  m_lqip_miss_logged = false;
  std::optional<surf::SoftwareSurface> discard;
  while (m_queue.try_pop(discard)) {
  }
}

} // namespace galapix

/* EOF */
