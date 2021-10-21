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

#include <surf/color.hpp>
#include <wstdisplay/graphics_context.hpp>

#include "galapix/image.hpp"
#include "galapix/image_tile_cache.hpp"
#include "math/math.hpp"
#include "math/rect.hpp"

using namespace surf;

ImageRenderer::ImageRenderer(Image& image, ImageTileCachePtr const& cache) :
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

void
ImageRenderer::draw_tile(wstdisplay::GraphicsContext& gc, int x, int y, int scale, float zoom)
{
  ImageTileCache::SurfaceStruct sstruct = m_cache->request_tile(x, y, scale);
  if (sstruct.surface)
  {
    sstruct.surface->draw(gc, Rectf(get_vertex(x,   y,   zoom),
                                    get_vertex(x+1, y+1, zoom)));

    if ((false))
    { // draw debug rectangle that shows tiles
      gc.draw_rect(Rectf(get_vertex(x,   y,   zoom) + geom::fsize(zoom, zoom) * 8.0f,
                         get_vertex(x+1, y+1, zoom) - geom::fsize(zoom, zoom) * 8.0f),
                   surf::Color::from_rgb888(255, 0, 255));
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

        surface->draw(gc, subsection,
                      Rectf(get_vertex(x,   y,   zoom),
                            get_vertex(x+1, y+1, zoom)));
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
ImageRenderer::draw_tiles(wstdisplay::GraphicsContext& gc, const Rect& rect, int scale, float zoom)
{
  for(int y = rect.top(); y < rect.bottom(); ++y) {
    for(int x = rect.left(); x < rect.right(); ++x) {
      draw_tile(gc, x, y, scale, zoom);
    }
  }
}

bool
ImageRenderer::draw(wstdisplay::GraphicsContext& gc, const Rectf& cliprect, float zoom)
{
  Rectf image_rect = m_image.get_image_rect();

  if (!geom::intersects(cliprect, image_rect))
  {
    m_cache->cleanup();
    return false;
  }
  else
  {
    // scale factor for requesting the tile from the TileDatabase
    // FIXME: Can likely be done without float
    int tiledb_scale = std::clamp(static_cast<int>(logf(1.0f / (zoom * m_image.get_scale())) /
                                                   logf(2.0f)),
                                  0, m_cache->get_max_scale());
    int scale_factor = Math::pow2(tiledb_scale);

    int scaled_width  = m_image.get_original_width()  / scale_factor;
    int scaled_height = m_image.get_original_height() / scale_factor;

    if (scaled_width  < 256 && scaled_height < 256)
    { // So small that only one tile is to be drawn
      m_cache->cancel_jobs(Rect(0,0,1,1), tiledb_scale);
      draw_tile(gc, 0, 0, tiledb_scale,
                static_cast<float>(scale_factor) * m_image.get_scale());
    }
    else
    {
      Rectf image_region = geom::intersection(image_rect, cliprect); // visible part of the image

      image_region = Rectf((image_region.left()   - image_rect.left()) / m_image.get_scale(),
                           (image_region.top()    - image_rect.top())  / m_image.get_scale(),
                           (image_region.right()  - image_rect.left()) / m_image.get_scale(),
                           (image_region.bottom() - image_rect.top())  / m_image.get_scale());

      int   itilesize = 256 * scale_factor;

      int start_x = static_cast<int>(image_region.left() / static_cast<float>(itilesize));
      int end_x   = Math::ceil_div(static_cast<int>(image_region.right()), itilesize);

      int start_y = static_cast<int>(image_region.top() / static_cast<float>(itilesize));
      int end_y   = Math::ceil_div(static_cast<int>(image_region.bottom()), itilesize);

      Rect rect(start_x, start_y, end_x, end_y);
      m_cache->cancel_jobs(rect, tiledb_scale);
      draw_tiles(gc,
                 rect, tiledb_scale,
                 static_cast<float>(scale_factor) * m_image.get_scale());
    }

    return true;
  }
}

/* EOF */
