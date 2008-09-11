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

#include <boost/bind.hpp>
#include "math/rgb.hpp"
#include "math/rect.hpp"
#include "framebuffer.hpp"
#include "surface.hpp"
#include "math.hpp"
#include "file_entry.hpp"
#include "database_thread.hpp"
#include "viewer_thread.hpp"
#include "thread_message_queue.hpp"
#include "image.hpp"

uint32_t make_cache_id(int x, int y, int tile_scale)
{
  return x | (y << 8) | (tile_scale << 16);
}

class ImageImpl
{
public:
  FileEntry file_entry;

  int min_keep_scale; 

  float alpha;

  /** Position refers to the center of the image */
  Vector2f pos;
  Vector2f last_pos;
  Vector2f target_pos;

  /** Scale of the image */
  float scale;
  float last_scale;
  float target_scale;

  /** Rotation angle */
  float angle;

  Image::Cache cache;

  /** FIXME: Jobs array does not get cleared after jobs are done */
  Image::Jobs jobs;  

  ThreadMessageQueue<TileEntry> tile_queue;
  
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

Image::Image(const FileEntry& file_entry)
  : impl(new ImageImpl())
{
  impl->file_entry = file_entry;

  impl->angle = 0.0f;
  
  impl->alpha = 1.0f;

  impl->scale        = 1.0f;
  impl->last_scale   = 1.0f;
  impl->target_scale = 1.0f;

  int size  = Math::max(file_entry.get_width(), file_entry.get_height());
  impl->min_keep_scale = 0;
  while(size > 32) 
    {
      size /= 2;
      impl->min_keep_scale +=1 ;
    }
  
}

void
Image::set_alpha(float alpha)
{
  impl->alpha = alpha;
}

float
Image::get_alpha() const
{
  return impl->alpha;
}

Vector2f
Image::get_top_left_pos() const
{
  return impl->pos - Vector2f(get_scaled_width()/2, get_scaled_height()/2);
}

void
Image::set_pos(const Vector2f& pos)
{
  impl->pos        = pos;
  impl->last_pos   = pos;
  impl->target_pos = pos;
}

Vector2f
Image::get_pos() const
{
  return impl->pos;
}

void
Image::set_target_pos(const Vector2f& target_pos)
{
  impl->last_pos   = impl->pos;
  impl->target_pos = target_pos;
}

void
Image::set_target_scale(float target_scale)
{
  impl->last_scale   = impl->scale;
  impl->target_scale = target_scale;  
}

void
Image::update_pos(float progress)
{
  assert(progress >= 0.0f &&
         progress <= 1.0f);

  if (progress == 1.0f)
    {
      set_pos(impl->target_pos);
      set_scale(impl->target_scale);
    }
  else
    {
      impl->pos   = (impl->last_pos   * (1.0f - progress)) + (impl->target_pos   * progress);
      impl->scale = (impl->last_scale * (1.0f - progress)) + (impl->target_scale * progress);
    }
}

void
Image::set_angle(float a)
{
  impl->angle = a;
}

float
Image::get_angle(float a) const
{
  return impl->angle;
}

void
Image::set_scale(float f)
{
  impl->scale        = f;
  impl->last_scale   = f;
  impl->target_scale = f;
}

float
Image::get_scale() const
{
  return impl->scale;
}

float
Image::get_scaled_width() const
{
  return impl->file_entry.get_width() * impl->scale;
}

float
Image::get_scaled_height() const
{
  return impl->file_entry.get_height() * impl->scale;
}

int
Image::get_original_width() const
{
  return impl->file_entry.get_width();
}

int
Image::get_original_height() const
{
  return impl->file_entry.get_height();
}

Surface
Image::get_tile(int x, int y, int tile_scale)
{
  uint32_t cache_id = make_cache_id(x, y, tile_scale);
  Cache::iterator i = impl->cache.find(cache_id);

  if (i == impl->cache.end())
    {
      // Important: it must be '*this', not 'this', since the 'this'
      // pointer might disappear any time, its only the impl that
      // stays and which we can link to by making a copy of the Image
      // object via *this.
      //std::cout << "  Requesting: " << impl->file_entry.size << " " << x << "x" << y << " scale: " << tile_scale << std::endl;
      impl->jobs.push_back(DatabaseThread::current()->request_tile(impl->file_entry, tile_scale, Vector2i(x, y), 
                                                                   boost::bind(&Image::receive_tile, *this, _1)));

      // FIXME: Something to try: Request the next smaller tile too,
      // so we get a lower quality image fast and a higher quality one
      // soon after FIXME: Its unclear if this actually improves
      // things, also the order of request gets mungled in the
      // DatabaseThread, we should request the whole group of lower
      // res tiles at once, instead of one by one, since that eats up
      // the possible speed up
      //impl->jobs.push_back(DatabaseThread::current()->request_tile(impl->file_entry, tile_scale+1, Vector2i(x, y), 
      //                                                             boost::bind(&Image::receive_tile, *this, _1)));

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

Surface
Image::find_smaller_tile(int x, int y, int tiledb_scale, int& downscale)
{
  int  downscale_factor = 1;

  // FIXME: Replace this loop with a 'find_next_best_smaller_tile()' function
  while(downscale_factor < 10) // Make this 'max_scale' instead of random number
    {
      downscale = Math::pow2(downscale_factor);
      uint32_t cache_id = make_cache_id(x/downscale, y/downscale, tiledb_scale+downscale_factor);
      Cache::iterator i = impl->cache.find(cache_id);
      if (i != impl->cache.end() && i->second.surface)
        {
          return i->second.surface;
        }

      downscale_factor += 1;
    }
  return Surface();
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
      int downscale;
      surface = find_smaller_tile(x, y, tiledb_scale, downscale);
      if (surface)
        { // Must only draw relevant section!
          Size s((x%downscale) ? (surface.get_width()  - 256/downscale * (x%downscale)) : 256/downscale,
                 (y%downscale) ? (surface.get_height() - 256/downscale * (y%downscale)) : 256/downscale);

          s.width  = Math::min(surface.get_width(),  s.width);
          s.height = Math::min(surface.get_height(), s.height);
          
          surface.draw(Rectf(Vector2f(x%downscale, y%downscale) * 256/downscale, 
                             s),
                       Rectf(pos, s * scale * downscale));
        }
      else // draw replacement rect when no tile could be loaded
        {         
          // Calculate the actual size of the tile (i.e. border tiles might be smaller then 256x256)
          Size s(Math::min(256, (impl->file_entry.get_width()  / Math::pow2(tiledb_scale)) - 256 * x),
                 Math::min(256, (impl->file_entry.get_height() / Math::pow2(tiledb_scale)) - 256 * y));

          Framebuffer::fill_rect(Rectf(pos, s*scale), RGB(155, 0, 155)); // impl->file_entry.color);
        }
    }
}

void 
Image::draw_tiles(const Rect& rect, int tiledb_scale, 
                  const Vector2f& pos, float scale)
{
  //std::cout << " drawtiles: " << rect << " scale: " << tiledb_scale << std::endl;

  float tilesize = 256.0f * scale;

  for(int y = rect.top; y < rect.bottom; ++y)
    for(int x = rect.left; x < rect.right; ++x)
      {
        draw_tile(x, y, tiledb_scale, 
                  get_top_left_pos() + Vector2f(x,y) * tilesize,
                  scale);
      }
}

void
Image::process_queue()
{
  // Check the queue for newly arrived tiles
  while (!impl->tile_queue.empty())
    {
      TileEntry tile = impl->tile_queue.front();
      impl->tile_queue.pop();

      int tile_id = make_cache_id(tile.pos.x, tile.pos.y, tile.scale);
  
      SurfaceStruct s;
  
      if (tile.surface)
        {
          s.surface = Surface(tile.surface);
          s.status  = SurfaceStruct::SURFACE_OK;
        }
      else
        {
          s.surface = Surface();
          s.status  = SurfaceStruct::SURFACE_FAILED;
        }

      impl->cache[tile_id] = s;
    }
}

void
Image::cache_cleanup()
{
  // FIXME: Cache cleanup should happen based on if the Tile is
  // visible, not if the image is visible

  // Image is not visible, so cancel all jobs
  for(Jobs::iterator i = impl->jobs.begin(); i != impl->jobs.end(); ++i)
    i->abort();
  impl->jobs.clear();
        
  // FIXME: We also need to purge the cache more often, since with
  // big gigapixel images we would end up never clearing it
      
  // Erase all tiles larger then 32x32

  // FIXME: Could make this more clever and relative to the number
  // of the images we display, since with large collections 32x32
  // might be to much for memory while with small collections it
  // will lead to unnecessary loading artifacts.      
  for(Cache::iterator i = impl->cache.begin(); i != impl->cache.end(); ++i)
    {
      int tiledb_scale = (i->first >> 16);
      if (tiledb_scale < impl->min_keep_scale)
        impl->cache.erase(i);
    }
}

void
Image::clear_cache()
{
 for(Jobs::iterator i = impl->jobs.begin(); i != impl->jobs.end(); ++i)
    i->abort();
  impl->jobs.clear();

  impl->cache.clear();
}

void
Image::print_info()
{
  std::cout << "  Image: " << impl.get() << std::endl;
  std::cout << "    Cache Size: " << impl->cache.size() << std::endl;
  std::cout << "    Job Size:   " << impl->jobs.size() << std::endl;
}

bool
Image::overlaps(const Vector2f& pos) const
{
  return get_image_rect().contains(pos);
}

bool
Image::overlaps(const Rectf& cliprect) const
{
  return cliprect.is_overlapped(get_image_rect());
}

void
Image::draw(const Rectf& cliprect, float fscale)
{
  process_queue();
  
  Rectf image_rect = get_image_rect();
  Vector2f top_left(image_rect.left, image_rect.top);

  if (!cliprect.is_overlapped(image_rect))
    {
      cache_cleanup();
    }
  else
    {
      // scale factor for requesting the tile from the TileDatabase
      // FIXME: Can likely be done without float
      int tiledb_scale = Math::max(0, static_cast<int>(log(1.0f / (fscale*impl->scale)) /
                                                       log(2)));
      int scale_factor = Math::pow2(tiledb_scale);

      int scaled_width  = impl->file_entry.get_width()  / scale_factor;
      int scaled_height = impl->file_entry.get_height() / scale_factor;

      if (scaled_width  < 256 && scaled_height < 256)
        { // So small that only one tile is to be drawn
          draw_tile(0, 0, tiledb_scale, 
                    top_left,
                    scale_factor * impl->scale);
        }
      else
        {
          Rectf image_region = image_rect.clip_to(cliprect); // visible part of the image

          image_region.left   = (image_region.left   - top_left.x) / impl->scale;
          image_region.right  = (image_region.right  - top_left.x) / impl->scale;
          image_region.top    = (image_region.top    - top_left.y) / impl->scale;
          image_region.bottom = (image_region.bottom - top_left.y) / impl->scale;

          int   itilesize = 256 * scale_factor;
          
          int start_x = image_region.left  / itilesize;
          int end_x   = Math::ceil_div(image_region.right, itilesize);

          int start_y = image_region.top / itilesize;
          int end_y   = Math::ceil_div(image_region.bottom, itilesize);

          draw_tiles(Rect(start_x, start_y, end_x, end_y), 
                     tiledb_scale, 
                     top_left,
                     scale_factor * impl->scale);
        }
    }
}

std::string
Image::get_filename() const
{
  return impl->file_entry.get_filename();
}

void
Image::draw_mark()
{
  Framebuffer::draw_rect(get_image_rect(), RGB(255, 255, 255));
}

Rectf
Image::get_image_rect() const
{
  Sizef image_size(impl->file_entry.get_size() * impl->scale);
  return Rectf(impl->pos - Vector2f(image_size.width/2, image_size.height/2), image_size); // in world coordinates
}

void
Image::receive_tile(const TileEntry& tile)
{
  assert(impl.get());
  impl->tile_queue.push(tile);
}

/* EOF */
