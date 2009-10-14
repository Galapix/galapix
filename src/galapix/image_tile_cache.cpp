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

#include "galapix/image_tile_cache.hpp"

#include <boost/bind.hpp>

#include "math/math.hpp"
#include "galapix/viewer.hpp"
#include "galapix/database_thread.hpp"

unsigned int make_cache_id(int x, int y, int scale)
{
  return x | (y << 8) | (scale << 16);
}

ImageTileCache::ImageTileCache(const FileEntry& file_entry_) :
  cache(),
  jobs(),
  tile_queue(),
  file_entry(file_entry_),
  max_scale(file_entry.get_thumbnail_scale()),
  min_keep_scale(max_scale - 2)
{
}

SurfaceHandle
ImageTileCache::get_tile(int x, int y, int scale)
{
  if (x < 0 || y < 0 || scale < 0)
  {
    return SurfaceHandle();
  }
  else
  {
    unsigned int cache_id = make_cache_id(x, y, scale);
    Cache::iterator i = cache.find(cache_id);

    if (i != cache.end())
    {
      return i->second.surface;
    }
    else
    {
      return SurfaceHandle();
    }
  }
}

ImageTileCache::SurfaceStruct
ImageTileCache::request_tile(int x, int y, int scale)
{
  unsigned int cache_id = make_cache_id(x, y, scale);
  Cache::iterator i = cache.find(cache_id);

  if (i == cache.end())
  {
    // Important: it must be '*this', not 'this', since the 'this'
    // pointer might disappear any time, its only the impl that
    // stays and which we can link to by making a copy of the Image
    // object via *this.
    // std::cout << "  Requesting: " << impl->file_entry.size << " " << x << "x" << y << " scale: " << scale << std::endl;
    jobs.push_back(DatabaseThread::current()->request_tile(file_entry, scale, Vector2i(x, y), 
                                                           boost::bind(&ImageTileCache::receive_tile, this, _1)));

    // FIXME: Something to try: Request the next smaller tile too,
    // so we get a lower quality image fast and a higher quality one
    // soon after FIXME: Its unclear if this actually improves
    // things, also the order of request gets mungled in the
    // DatabaseThread, we should request the whole group of lower
    // res tiles at once, instead of one by one, since that eats up
    // the possible speed up
    // impl->jobs.push_back(DatabaseThread::current()->request_tile(impl->file_entry, scale+1, Vector2i(x, y), 
    //                                                              boost::bind(&Image::receive_tile, *this, _1)));
    SurfaceStruct s;
      
    s.surface = SurfaceHandle();
    s.status  = SurfaceStruct::SURFACE_REQUESTED;

    cache[cache_id] = s;

    return s;
  }
  else
  {
    return i->second;
  }
}

void
ImageTileCache::clear()
{
  for(Jobs::iterator i = jobs.begin(); i != jobs.end(); ++i)
  {
    i->set_aborted();
  }
  jobs.clear();

  cache.clear();
}

void
ImageTileCache::cleanup()
{
  // FIXME: Cache cleanup should happen based on if the Tile is
  // visible, not if the image is visible

  // Image is not visible, so cancel all jobs
  for(Jobs::iterator i = jobs.begin(); i != jobs.end(); ++i)
  {
    i->set_aborted();
  }
  jobs.clear();
        
  // FIXME: We also need to purge the cache more often, since with
  // big gigapixel images we would end up never clearing it
      
  // Erase all tiles larger then 32x32

  // FIXME: Could make this more clever and relative to the number
  // of the images we display, since with large collections 32x32
  // might be to much for memory while with small collections it
  // will lead to unnecessary loading artifacts.      

  // FIXME: Code can hang here for some reason
  for(Cache::iterator i = cache.begin(); i != cache.end();)
  {
    int tiledb_scale = (i->first >> 16);
    if (tiledb_scale < min_keep_scale ||
        i->second.status == SurfaceStruct::SURFACE_REQUESTED)
    {
      cache.erase(i++);
    }
    else
    {
      ++i;
    }
  }
}

SurfaceHandle
ImageTileCache::find_smaller_tile(int x, int y, int tiledb_scale, int& downscale_out)
{
  int  downscale_factor = 1;

  while(downscale_factor < max_scale)
  {
    downscale_out = Math::pow2(downscale_factor);

    uint32_t cache_id = make_cache_id(x / downscale_out, y / downscale_out,
                                      tiledb_scale+downscale_factor);

    Cache::iterator i = cache.find(cache_id);
    if (i != cache.end() && i->second.surface)
    {
      return i->second.surface;
    }

    downscale_factor += 1;
  }

  return SurfaceHandle();
}

void
ImageTileCache::process_queue()
{
  // Check the queue for newly arrived tiles
  while (!tile_queue.empty())
  {
    TileEntry tile = tile_queue.front();
    tile_queue.pop();

    int tile_id = make_cache_id(tile.get_pos().x, tile.get_pos().y, tile.get_scale());
  
    SurfaceStruct s;
  
    if (tile.get_surface())
    {
      s.surface = Surface::create(tile.get_surface());
      s.status  = SurfaceStruct::SURFACE_OK;
    }
    else
    {
      s.surface = SurfaceHandle();
      s.status  = SurfaceStruct::SURFACE_FAILED;
    }

    cache[tile_id] = s;
  }
}

void
ImageTileCache::receive_tile(const TileEntry& tile)
{
  tile_queue.push(tile);

  Viewer::current()->redraw();
}

/* EOF */
