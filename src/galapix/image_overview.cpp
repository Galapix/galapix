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
#include <surf/pixel_format.hpp>
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
                                TileProviderPtr provider)
{
  if (m_state != State::Idle) {
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
  // Thumtoo: first soft layer is the **levels** ladder (JXL preview via
  // request_pixels), not a grid tile. Grid tiles are a later refinement.
  // Share the per-frame budget so first paint does not stampede the client.
  if (auto* tp = dynamic_cast<ThumtooTileProvider*>(provider.get())) {
    if (!ImageTileCache::tile_requests_enabled()) {
      return; // cache-only mode — stay Idle
    }
    if (!ImageTileCache::try_consume_request_budget()) {
      return; // stay Idle — retry next frame
    }
    m_state = State::Loading;
    m_job = JobHandle::create();
    constexpr int kOverviewMaxEdge = 512;
    auto client = tp->client();
    std::string uri = tp->uri();
    client->request_pixels(
      std::move(uri), kOverviewMaxEdge,
      [weak_self, job = m_job](std::string, int, std::optional<thumtoo::PixelLevel> px) {
        auto self = weak_self.lock();
        if (!self) {
          return;
        }
        if (job.is_aborted()) {
          return;
        }
        if (!px || px->bytes.empty() || px->width <= 0 || px->height <= 0) {
          self->receive_software(std::nullopt);
          job.set_failed();
          return;
        }
        try {
          // JXL (or whatever codec thumtoo stored) via surface factory.
          auto surface = g_app.surface_factory().from_mem(
            std::span<uint8_t const>(px->bytes.data(), px->bytes.size()),
            px->codec == "jxl" ? "image/jxl" : px->codec,
            "thumtoo-level");
          if (surface.get_width() <= 0) {
            self->receive_software(std::nullopt);
            job.set_failed();
            return;
          }
          self->receive_software(std::move(surface));
          job.set_finished();
        } catch (...) {
          self->receive_software(std::nullopt);
          job.set_failed();
        }
      });
    return;
  }
#endif

  // Local files via JobManager + libjpeg DCT scale.
  if (!job_manager || !url.has_stdio_name()) {
    m_state = State::Failed;
    return;
  }

  m_state = State::Loading;
  m_job = JobHandle::create();

  int const min_scale = min_scale_for_overview(original_width, original_height);

  auto job = std::make_shared<OverviewLoadJob>(
    m_job, url, min_scale,
    [weak_self](std::optional<surf::SoftwareSurface> surface) {
      if (auto self = weak_self.lock()) {
        self->receive_software(std::move(surface));
      }
    });

  job_manager->request(job);
}

void
ImageOverview::receive_software(std::optional<surf::SoftwareSurface> surface)
{
  // Worker thread: only enqueue for main-thread GL upload.
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
    m_state = State::Failed;
    m_surface.reset();
    return;
  }

  try {
    m_surface = surface_from_software(std::move(*surface));
    m_state = State::Ready;
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
  std::optional<surf::SoftwareSurface> discard;
  while (m_queue.try_pop(discard)) {
  }
}

} // namespace galapix

/* EOF */
