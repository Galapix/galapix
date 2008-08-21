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

#include "math/rgb.hpp"
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
  Image::Jobs jobs;  
  
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
      impl->jobs.push_back(ViewerThread::current()->request_tile(impl->fileid, tile_scale, x, y, *this));

      // Request the next smaller tile too, so we get a lower quality
      // image fast and a higher quality one soon after FIXME: Its
      // unclear if this actually improves things, also the order of
      // request gets mungled in the DatabaseThread, we should request
      // the whole group of lower res tiles at once, instead of one by
      // one, since that eats up the possible speed up
      impl->jobs.push_back(ViewerThread::current()->request_tile(impl->fileid, tile_scale+1, x, y, *this));

      SurfaceStruct s;
      
      s.surface = Surface();
      s.status  = SurfaceStruct::SURFACE_REQUESTED;

      impl->cache[cache_id] = s;

      return Surface();
    }
  else
    {
      return i->second.surface;
    }
}

void
Image::draw_tile(int x, int y, int tiledb_scale, 
                 const Vector2f& pos, float scale)
{
  Surface surface = get_tile(x, y, tiledb_scale);
  if (surface)
    {
      surface.draw(Rectf(pos, surface.get_size() * scale));
      //Framebuffer::draw_rect(Rectf(pos, surface.get_size() * scale), RGB(100, 100, 100));
    }
  else
    {
      // Look for the next smaller tile
      // FIXME: Rewrite this to work all smaller tiles, not just the next
      
      int downscale_factor = 1;

    retry:
      int downscale = Math::pow2(downscale_factor);

      uint32_t cache_id = make_cache_id(x/downscale, y/downscale, tiledb_scale+downscale_factor);
      Cache::iterator i = impl->cache.find(cache_id);
  
      if (i != impl->cache.end() && i->second.surface)
        { // Must only draw relevant section!
          Size s((x%downscale) ? (i->second.surface.get_width()  - 256/downscale * (x%downscale)) : 256/downscale,
                 (y%downscale) ? (i->second.surface.get_height() - 256/downscale * (y%downscale)) : 256/downscale);

          s.width  = Math::min(i->second.surface.get_width(),  s.width);
          s.height = Math::min(i->second.surface.get_height(), s.height);
          
          i->second.surface.draw(Rectf(Vector2f(x%downscale, y%downscale) * 256/downscale, 
                                       s),
                                 Rectf(pos, s * scale * downscale));
          //Framebuffer::draw_rect(Rectf(pos, s * scale * downscale), RGB(255, 255, 255));
        }
      else
        {
          if (downscale_factor < 6) // Make this 'max_scale' instead of random number
            {
              downscale_factor += 1;
              goto retry;
            }
          else
            {         
              // give up, no lower resolution found

              Size s(Math::min(256, (impl->size.width  / Math::pow2(tiledb_scale)) - 256 * x),
                     Math::min(256, (impl->size.height / Math::pow2(tiledb_scale)) - 256 * y));

              Framebuffer::fill_rect(Rectf(pos, s*scale),
                                     RGB(255, 0, 255));
            }
        }
    }
}

void
Image::draw(const Rectf& cliprect, float fscale)
{
  // Cancel all old jobs (FIXME: Stupid brute force hack)
  if (0)
    {
      for(Jobs::iterator i = impl->jobs.begin(); i != impl->jobs.end(); ++i)
        i->abort();
      impl->jobs.clear();
    }

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
      
      // Clear the cache, but keep the smallest tile (Wonky hack)
      if (0)
        {
          impl->cache.clear();
        }
      else
        {
          int max_tiledb_scale = 0;
          SurfaceStruct s;
          int tileid;
          for(Cache::iterator i = impl->cache.begin(); i != impl->cache.end(); ++i)
            {
              int tiledb_scale = (i->first >> 16);
              if (tiledb_scale > max_tiledb_scale)
                {
                  max_tiledb_scale = tiledb_scale;
                  tileid = i->first;
                  s      = i->second;
                }
            }
          impl->cache.clear();

          if (max_tiledb_scale != 0)
            {
              impl->cache[tileid] = s;
            }
        }
    }
}

void
Image::receive_tile(int x, int y, int tiledb_scale, const SoftwareSurface& surface)
{
  int tile_id = make_cache_id(x, y, tiledb_scale);

  SurfaceStruct s;
  
  s.surface = Surface(surface);
  s.status  = SurfaceStruct::SURFACE_OK;

  impl->cache[tile_id] = s;
}

/* EOF */
