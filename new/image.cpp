/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#include "math/rect.hpp"
#include "framebuffer.hpp"
#include "surface.hpp"
#include "math.hpp"
#include "tile_database.hpp"
#include "image.hpp"

uint32_t make_cache_id(int x, int y, int tile_scale)
{
  return x | (y << 8) | (tile_scale << 16);
}

Image::Image(int fileid, const std::string& filename, const Size& size)
  : fileid(fileid),
    filename(filename),
    size(size),
    scale(1.0f)
{
  int tiledb_scale = 0;
  Size tmpsize = size;
  do {
    tmpsize.width  /= 2;
    tmpsize.height /= 2;
    tiledb_scale += 1;
  } while (tmpsize.width  > 32 ||
           tmpsize.height > 32);
  max_tiledb_scale = tiledb_scale;
}

void
Image::set_pos(const Vector2f& pos_)
{
  pos = pos_;
}

Vector2f
Image::get_pos() const
{
  return pos;
}

void
Image::set_scale(float f)
{
  scale = f;
}

float
Image::get_scale() const
{
  return scale;
}

float
Image::get_width() const
{
  return size.width;
}

float
Image::get_height() const
{
  return size.height;
}

Surface
Image::get_tile(int x, int y, int tile_scale)
{
  uint32_t cache_id = make_cache_id(x, y, tile_scale);
  Cache::iterator i = cache.find(cache_id);

  if (i == cache.end())
    {
      Tile tile;
      if (TileDatabase::current()->get_tile(fileid, tile_scale, x, y, tile))
        {                   
          Surface surface(tile.surface);
          cache[cache_id] = surface;
          return surface;
        }
      else
        {
          return Surface();
        }
    }
  else
    {
      return i->second;
    }
}

void
Image::draw(const Rectf& cliprect, float fscale)
{
  Rectf image_rect(pos, Sizef(size * scale)); // in world coordinates

  //Framebuffer::draw_rect(image_rect);
  //Framebuffer::draw_rect(image_region);

  if (cliprect.is_overlapped(image_rect))
    {
      Rectf image_region = image_rect.clip_to(cliprect); // visible part of the image

      // scale factor for requesting the tile from the TileDatabase
      int tiledb_scale = Math::max(0, static_cast<int>(log(1.0f / (fscale)) / log(2)));
      int scale_factor = Math::pow2(tiledb_scale);

      int scaled_width  = image_rect.get_width()  / scale_factor;
      int scaled_height = image_rect.get_height() / scale_factor;

      if (scaled_width  < 256 && scaled_height < 256)
        { // So small that only one tile is to be drawn
          //Framebuffer::draw_rect(Rectf(pos, size));
          get_tile(0, 0, Math::min(max_tiledb_scale, tiledb_scale)).draw(image_rect);
        }
      else
        {
          int tilesize = 256 * scale_factor;

          int start_x = (image_region.left  - pos.x)/tilesize;
          int end_x   = (image_region.right - pos.x)/tilesize + 1;

          int start_y = (image_region.top    - pos.y)/tilesize;
          int end_y   = (image_region.bottom - pos.y)/tilesize + 1;

          for(int y = start_y; y < end_y; y += 1)
            for(int x = start_x; x < end_x; x += 1)
              {
                Surface surface = get_tile(x, y, tiledb_scale);
                surface.draw(Rectf(pos.x + (x * tilesize), 
                                   pos.y + (y * tilesize), 
                                   pos.x + ((x * tilesize) + (surface.get_width()  * scale_factor)),
                                   pos.y + ((y * tilesize) + (surface.get_height() * scale_factor))));

                Framebuffer::draw_rect(Rectf(pos + Vector2f(x*tilesize, y*tilesize),
                                             Sizef(tilesize, tilesize)));
              }
        }
    }
  else
    {
      // Image is not visible so clear the cache
      // FIXME: We should keep at least some tiles or wait with the cache purge a bit longer
      cache.clear();
    }
}

/* EOF */
