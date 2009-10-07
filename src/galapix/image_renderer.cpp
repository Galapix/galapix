/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "galapix/image_renderer.hpp"

#include "display/framebuffer.hpp"
#include "display/surface.hpp"
#include "galapix/image.hpp"
#include "galapix/image_tile_cache.hpp"
#include "math/math.hpp"
#include "math/rect.hpp"
#include "math/rgb.hpp"
#include "math/rgba.hpp"

ImageRenderer::ImageRenderer(ImageTileCache& cache)
  : m_cache(cache)
{
}

void
ImageRenderer::draw_tile(int x, int y, int scale, 
                         const Vector2f& pos, float zoom, 
                         Image& m_image)
{
  Surface surface = m_cache.get_tile(x, y, scale);
  if (surface)
  {
    // FIXME: surface.get_size() * scale does not give the correct
    // size of a tile due to rounding errors
    surface.draw(Rectf(pos, surface.get_size() * zoom));
    //surface.draw(Rectf(pos, get_tile_size(x, y, scale)));
    //Framebuffer::draw_rect(Rectf(pos, surface.get_size() * scale), RGB(100, 100, 100));
  }
  else
  {
    // Look for the next smaller tile
    // FIXME: Rewrite this to work all smaller tiles, not just the next     
    int downscale;
    surface = m_cache.find_smaller_tile(x, y, scale, downscale);

    // Calculate the actual size of the tile (i.e. border tiles might be smaller then 256x256)
    Size tile_size(Math::min(256, (m_image.get_original_width()  / Math::pow2(scale)) - 256 * x),
                   Math::min(256, (m_image.get_original_height() / Math::pow2(scale)) - 256 * y));


    if (surface)
    { // Must only draw relevant section!
      Size s((x%downscale) ? (surface.get_width()  - 256/downscale * (x%downscale)) : 256/downscale,
             (y%downscale) ? (surface.get_height() - 256/downscale * (y%downscale)) : 256/downscale);

      s.width  = Math::min(surface.get_width(),  s.width);
      s.height = Math::min(surface.get_height(), s.height);
          
      surface.draw(Rectf(Vector2f(static_cast<float>(x % downscale), 
                                  static_cast<float>(y % downscale)) * static_cast<float>(256/downscale), 
                         s),
                   //Rectf(pos, tile_size * scale)); kind of works, but leads to discontuinity and jumps
                   Rectf(pos, s * zoom * static_cast<float>(downscale)));
    }
    else // draw replacement rect when no tile could be loaded
    {         
      Framebuffer::fill_rect(Rectf(pos, tile_size * zoom), RGB(155, 0, 155)); // impl->file_entry.color);
    }

    //Framebuffer::draw_rect(Rectf(pos, s*scale), RGB(255, 255, 255)); // impl->file_entry.color);
  }
}

void 
ImageRenderer::draw_tiles(const Rect& rect, int scale, 
                          const Vector2f& pos, float zoom, 
                          Image& m_image)
{
  float tilesize = 256.0f * zoom;

  for(int y = rect.top; y < rect.bottom; ++y)
    for(int x = rect.left; x < rect.right; ++x)
    {
      draw_tile(x, y, scale, 
                m_image.get_top_left_pos() + Vector2f(static_cast<float>(x), static_cast<float>(y)) * tilesize,
                zoom,
                m_image);
    }
}

bool
ImageRenderer::draw(const Rectf& cliprect, float zoom, Image& m_image)
{
  Rectf image_rect = m_image.get_image_rect();

  if (!cliprect.is_overlapped(image_rect))
  {
    m_cache.cleanup();
    return false;
  }
  else
  {
    Vector2f top_left(image_rect.left, image_rect.top);

    // scale factor for requesting the tile from the TileDatabase
    // FIXME: Can likely be done without float
    int tiledb_scale = Math::clamp(0, static_cast<int>(log(1.0f / (zoom * m_image.get_scale())) /
                                                       log(2)), m_cache.get_max_scale());
    int scale_factor = Math::pow2(tiledb_scale);

    int scaled_width  = m_image.get_original_width()  / scale_factor;
    int scaled_height = m_image.get_original_height() / scale_factor;

    if (scaled_width  < 256 && scaled_height < 256)
    { // So small that only one tile is to be drawn
      draw_tile(0, 0, tiledb_scale, 
                top_left,
                static_cast<float>(scale_factor) * m_image.get_scale(),
                m_image);
    }
    else
    {
      Rectf image_region = image_rect.clip_to(cliprect); // visible part of the image

      image_region.left   = (image_region.left   - top_left.x) / m_image.get_scale();
      image_region.right  = (image_region.right  - top_left.x) / m_image.get_scale();
      image_region.top    = (image_region.top    - top_left.y) / m_image.get_scale();
      image_region.bottom = (image_region.bottom - top_left.y) / m_image.get_scale();

      int   itilesize = 256 * scale_factor;
          
      int start_x = static_cast<int>(image_region.left / static_cast<float>(itilesize));
      int end_x   = Math::ceil_div(static_cast<int>(image_region.right), itilesize);

      int start_y = static_cast<int>(image_region.top / static_cast<float>(itilesize));
      int end_y   = Math::ceil_div(static_cast<int>(image_region.bottom), itilesize);

      draw_tiles(Rect(start_x, start_y, end_x, end_y), 
                 tiledb_scale, 
                 top_left,
                 static_cast<float>(scale_factor) * m_image.get_scale(),
                 m_image);
    }

    return true;
  }
}

/* EOF */
