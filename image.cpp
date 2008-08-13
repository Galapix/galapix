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
#include "database_thread.hpp"
#include "viewer_thread.hpp"
#include "image.hpp"

uint32_t make_cache_id(int x, int y, int tile_scale)
{
  return x | (y << 8) | (tile_scale << 16);
}

class ImageImpl
{
public:
  int fileid;
  std::string filename;
  Size size;
  float scale;

  int max_tiledb_scale;
  Vector2f pos;

  Image::Cache cache;
  
  ImageImpl() 
  {
  }

  ~ImageImpl()
  {
  }
};

Image::Image()
{
}

Image::Image(int fileid, const std::string& filename, const Size& size)
  : impl(new ImageImpl())
{
  impl->fileid   = fileid;
  impl->filename = filename;
  impl->size     = size;
  impl->scale    = 1.0f;
  
  int  tiledb_scale = 0;
  Size tmpsize = size;
  do {
    tmpsize.width  /= 2;
    tmpsize.height /= 2;
    tiledb_scale += 1;
  } while (tmpsize.width  > 32 ||
           tmpsize.height > 32);

  impl->max_tiledb_scale = tiledb_scale;
}

void
Image::set_pos(const Vector2f& pos_)
{
  impl->pos = pos_;
}

Vector2f
Image::get_pos() const
{
  return impl->pos;
}

void
Image::set_scale(float f)
{
  impl->scale = f;
}

float
Image::get_scale() const
{
  return impl->scale;
}

float
Image::get_scaled_width() const
{
  return impl->size.width * impl->scale;
}

float
Image::get_scaled_height() const
{
  return impl->size.height * impl->scale;
}

int
Image::get_original_width() const
{
  return impl->size.width;
}

int
Image::get_original_height() const
{
  return impl->size.height;
}

Surface
Image::get_tile(int x, int y, int tile_scale)
{
  uint32_t cache_id = make_cache_id(x, y, tile_scale);
  Cache::iterator i = impl->cache.find(cache_id);

  if (i == impl->cache.end())
    {
      ViewerThread::current()->request_tile(impl->fileid, tile_scale, x, y, *this);

      // FIXME: Insert code here to find the next best tile
      //return impl->cache[cache_id] = Surface(); // We add an empty surface, so we don't do duplicate requests
      return impl->cache[cache_id] = get_next_best_tile(x, y, tile_scale);
    }
  else
    {
      return i->second;
    }
}

Surface
Image::get_next_best_tile(int x, int y, int tile_scale)
{
  uint32_t cache_id = make_cache_id(x/2, y/2, tile_scale+1);
  Cache::iterator i = impl->cache.find(cache_id);
  
  if (i == impl->cache.end() || !i->second)
    {
      // No tile at smaller zoom level found, so give up
      return Surface();
    }
  else
    {
      // FIXME: This can't handle tiles, which aren't dim sized this
      // way, need to do something with the image width to figure out
      // how large the tile should really be
      return Surface(); //return i->second.get_quadrant(x % 2, y % 2);
    }
}

void
Image::draw_tile(int x, int y, int tiledb_scale, const Vector2f& pos, float scale)
{
  Surface surface = get_tile(x, y, tiledb_scale);
  if (surface)
    {
      surface.draw(Rectf(pos, surface.get_size() * scale));

    }
  else
    {
      //Framebuffer::draw_rect(Rectf(pos + Vector2f(x, y) * tilesize,
      //                         Sizef(tilesize, tilesize)));
    }
}

void
Image::draw(const Rectf& cliprect, float fscale)
{
  Rectf image_rect(impl->pos, Sizef(impl->size * impl->scale)); // in world coordinates

  //Framebuffer::draw_rect(image_rect);

  if (cliprect.is_overlapped(image_rect))
    {
      // scale factor for requesting the tile from the TileDatabase
      int tiledb_scale = Math::max(0, static_cast<int>(log(1.0f / (fscale*impl->scale)) /
                                                       log(2)));
      int scale_factor = Math::pow2(tiledb_scale);

      int scaled_width  = impl->size.width  / scale_factor;
      int scaled_height = impl->size.height / scale_factor;

      if (scaled_width  < 256 && scaled_height < 256)
        { // So small that only one tile is to be drawn
          draw_tile(0, 0, tiledb_scale, 
                    impl->pos,
                    scale_factor * impl->scale);
        }
      else
        {
          Rectf image_region = image_rect.clip_to(cliprect); // visible part of the image

          image_region.left   = (image_region.left   - impl->pos.x) / impl->scale;
          image_region.right  = (image_region.right  - impl->pos.x) / impl->scale;
          image_region.top    = (image_region.top    - impl->pos.y) / impl->scale;
          image_region.bottom = (image_region.bottom - impl->pos.y) / impl->scale;

          int   itilesize = 256 * scale_factor;
          float tilesize  = 256.0f * scale_factor * impl->scale;

          int start_x = (image_region.left)  / itilesize;
          int end_x   = (image_region.right) / itilesize + 1;

          int start_y = (image_region.top   ) / itilesize;
          int end_y   = (image_region.bottom) / itilesize + 1;

          for(int y = start_y; y < end_y; y += 1)
            for(int x = start_x; x < end_x; x += 1)
              {
                draw_tile(x, y, tiledb_scale, 
                          impl->pos + Vector2f(x,y) * tilesize,
                          scale_factor * impl->scale);
              }
        }
    }
  else
    {
      // Image is not visible so clear the cache
      
      // FIXME: We should keep at least some tiles or wait with the
      // cache purge a bit longer

      // FIXME: We also need to purge the cache more often, since with
      // big images we would end up never clearing it
      impl->cache.clear();
    }
}

void
Image::receive_tile(int x, int y, int tiledb_scale, const SoftwareSurface& surface)
{
  int tile_id = make_cache_id(x, y, tiledb_scale);
  impl->cache[tile_id] = Surface(surface);
}

/* EOF */
