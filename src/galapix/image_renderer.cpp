// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "galapix/image_renderer.hpp"

#include <cmath>
#include <surf/color.hpp>
#include <wstdisplay/graphics_context.hpp>

#include "galapix/image.hpp"
#include "galapix/image_tile_cache.hpp"
#include "math/math.hpp"
#include "math/rect.hpp"

namespace galapix {

using namespace surf;

ImageRenderer::ImageRenderer(Image& image, std::shared_ptr<ImageTileCache> const& cache) :
  m_image(image),
  m_cache(cache)
{
}

Vector2f
ImageRenderer::get_vertex(int x, int y, float zoom) const
{
  float tilesize = 256.0f * zoom;

  return m_image.get_top_left_pos() +
    geom::fsize(std::min(static_cast<float>(x) * tilesize,
                         m_image.get_scaled_width()),
                std::min(static_cast<float>(y) * tilesize,
                         m_image.get_scaled_height()));
}

ImageRenderer::ViewPlan
ImageRenderer::plan_view(Rectf const& cliprect, float zoom)
{
  ViewPlan plan;
  Rectf image_rect = m_image.get_image_rect();

  if (!geom::intersects(cliprect, image_rect)) {
    return plan; // visible = false
  }
  plan.visible = true;

  int const desired_scale = std::clamp(
    static_cast<int>(std::floor(
      std::log(1.0f / (zoom * m_image.get_scale())) / std::log(2.0f) + 1e-5f)),
    m_cache->get_min_scale(), m_cache->get_max_scale());
  plan.tiledb_scale = m_cache->stable_request_scale(desired_scale);
  plan.scale_factor = std::ldexp(1.0f, plan.tiledb_scale);
  plan.tile_zoom = plan.scale_factor * m_image.get_scale();

  float const scaled_width  = static_cast<float>(m_image.get_original_width())  / plan.scale_factor;
  float const scaled_height = static_cast<float>(m_image.get_original_height()) / plan.scale_factor;

  if (scaled_width < 256.0f && scaled_height < 256.0f) {
    plan.one_cell = true;
    auto const& ov = m_image.overview();
    // Gallery / fit-all: prefer overview; only fall back to a grid tile when
    // overview Failed (same policy as pre-split draw path).
    if (ov.state() == ImageOverview::State::Ready && ov.has_surface()) {
      plan.skip_grid = true;
    } else if (ov.state() != ImageOverview::State::Failed) {
      plan.skip_grid = true;
    } else {
      plan.tile_rect = Rect(0, 0, 1, 1);
    }
  } else {
    Rectf image_region = geom::intersection(image_rect, cliprect);
    image_region = Rectf((image_region.left()   - image_rect.left()) / m_image.get_scale(),
                         (image_region.top()    - image_rect.top())  / m_image.get_scale(),
                         (image_region.right()  - image_rect.left()) / m_image.get_scale(),
                         (image_region.bottom() - image_rect.top())  / m_image.get_scale());

    float const itilesize = 256.0f * plan.scale_factor;
    int start_x = static_cast<int>(std::floor(image_region.left() / itilesize));
    int end_x   = static_cast<int>(std::ceil(image_region.right() / itilesize));
    int start_y = static_cast<int>(std::floor(image_region.top() / itilesize));
    int end_y   = static_cast<int>(std::ceil(image_region.bottom() / itilesize));
    plan.tile_rect = Rect(start_x, start_y, end_x, end_y);
  }

  return plan;
}

void
ImageRenderer::prepare(Rectf const& cliprect, float zoom)
{
  ViewPlan const plan = plan_view(cliprect, zoom);

  if (!plan.visible) {
    m_cache->cleanup();
    return;
  }

  // Soft overview load (may consume request budget for thumtoo path).
  m_image.overview().ensure_requested(
    m_image.job_manager(),
    m_image.get_url(),
    m_image.get_original_width(),
    m_image.get_original_height(),
    m_image.get_tile_provider());

  if (plan.skip_grid) {
    return;
  }

  if (plan.one_cell) {
    m_cache->cancel_jobs(Rect(0, 0, 1, 1), plan.tiledb_scale);
    m_cache->mark_tile_needed(0, 0, plan.tiledb_scale);
    return;
  }

  m_cache->cancel_jobs(plan.tile_rect, plan.tiledb_scale);
  for (int y = plan.tile_rect.top(); y < plan.tile_rect.bottom(); ++y) {
    for (int x = plan.tile_rect.left(); x < plan.tile_rect.right(); ++x) {
      m_cache->mark_tile_needed(x, y, plan.tiledb_scale);
    }
  }
}

void
ImageRenderer::draw_tile(wstdisplay::GraphicsContext& gc, int x, int y, int scale, float zoom)
{
  // Pure lookup — requests were issued in prepare / issue_requests.
  ImageTileCache::SurfaceStruct sstruct = m_cache->lookup_tile(x, y, scale);
  Rectf const tile_rect(get_vertex(x,   y,   zoom),
                        get_vertex(x+1, y+1, zoom));
  if (sstruct.surface)
  {
    sstruct.surface->draw(gc, tile_rect);

    if (ImageTileCache::tile_debug())
    {
      // Green: pixel data at the requested scale is on screen.
      gc.draw_rect(tile_rect, surf::Color::from_rgb888(0, 220, 0));
    }
  }
  else // tile not found, so find a replacement
  {
    // higher resolution tiles (FIXME: we are only using one level, should check everything recursivly)
   wstdisplay::SurfacePtr nw = m_cache->get_tile(2*x,   2*y,   scale - 1);
    wstdisplay::SurfacePtr ne = m_cache->get_tile(2*x+1, 2*y,   scale - 1);
    wstdisplay::SurfacePtr sw = m_cache->get_tile(2*x,   2*y+1, scale - 1);
    wstdisplay::SurfacePtr se = m_cache->get_tile(2*x+1, 2*y+1, scale - 1);

    if (!nw || !ne || !sw || !se)
    {
      // draw lower resolution tiles
      int downscale;
      wstdisplay::SurfacePtr surface = m_cache->find_smaller_tile(x, y, scale, downscale);

      if (surface)
      {
        Rectf subsection(Vector2f(static_cast<float>(x % downscale * 256 / downscale), // NOLINT
                                  static_cast<float>(y % downscale * 256 / downscale)), // NOLINT
                         Sizef(Size(256 / downscale, 256 / downscale)));

        subsection = geom::frect(subsection.topleft(),
                                 geom::fpoint(std::min(subsection.right(),  surface->get_width()),
                                              std::min(subsection.bottom(), surface->get_height())));

        surface->draw(gc, subsection, tile_rect);
        if (ImageTileCache::tile_debug()) {
          // Cyan: showing coarser stand-in (not the requested scale) — upscaled.
          gc.draw_rect(tile_rect, surf::Color::from_rgb888(0, 200, 255));
        }
      }
      else // draw replacement rect when no tile could be loaded
      {
        switch (sstruct.status)
        {
          case ImageTileCache::SurfaceStruct::SURFACE_SUCCEEDED:
            //assert(false && "should never happen");
            break;

          case ImageTileCache::SurfaceStruct::SURFACE_REQUESTED:
            gc.fill_rect(Rectf(get_vertex(x,   y,   zoom),
                                         get_vertex(x+1, y+1, zoom)),
                                   surf::Color::from_rgb888(155, 0, 155));
            break;

          default:
            assert(false && "should never happen either");
            break;
        }
      }
    }

    // draw higher resolution tiles
    if (nw) { nw->draw(gc, Rectf(get_vertex(2*x+0, 2*y+0, zoom/2.0f), get_vertex(2*x+1, 2*y+1, zoom/2.0f))); }
    if (ne) { ne->draw(gc, Rectf(get_vertex(2*x+1, 2*y+0, zoom/2.0f), get_vertex(2*x+2, 2*y+1, zoom/2.0f))); }
    if (sw) { sw->draw(gc, Rectf(get_vertex(2*x+0, 2*y+1, zoom/2.0f), get_vertex(2*x+1, 2*y+2, zoom/2.0f))); }
    if (se) { se->draw(gc, Rectf(get_vertex(2*x+1, 2*y+1, zoom/2.0f), get_vertex(2*x+2, 2*y+2, zoom/2.0f))); }
  }
}

void
ImageRenderer::draw_tiles(wstdisplay::GraphicsContext& gc, Rect const& rect, int scale, float zoom)
{
  for(int y = rect.top(); y < rect.bottom(); ++y) {
    for(int x = rect.left(); x < rect.right(); ++x) {
      draw_tile(gc, x, y, scale, zoom);
    }
  }
}

bool
ImageRenderer::draw(wstdisplay::GraphicsContext& gc, Rectf const& cliprect, float zoom)
{
  ViewPlan const plan = plan_view(cliprect, zoom);

  if (!plan.visible) {
    return false;
  }

  Rectf image_rect = m_image.get_image_rect();
  // Soft whole-image preview under tiles (load was requested in prepare).
  m_image.overview().draw(gc, image_rect);

  if (plan.skip_grid) {
    return true;
  }

  if (plan.one_cell) {
    draw_tile(gc, 0, 0, plan.tiledb_scale, plan.tile_zoom);
  } else {
    draw_tiles(gc, plan.tile_rect, plan.tiledb_scale, plan.tile_zoom);
  }

  return true;
}

} // namespace galapix

/* EOF */
