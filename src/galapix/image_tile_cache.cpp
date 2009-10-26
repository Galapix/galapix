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
#include <assert.h>

#include "util/weak_functor.hpp"
#include "math/math.hpp"
#include "galapix/viewer.hpp"
#include "galapix/database_thread.hpp"

unsigned int make_cache_id(int x, int y, int scale)
{
  assert(0 <= x && x < 4096);
  assert(0 <= y && y < 4096);
  assert(0 <= scale && scale < 256);

  return x | (y << 12) | (scale << 24);
}

ImageTileCachePtr
ImageTileCache::create(TileProviderPtr tile_provider)
{
  ImageTileCachePtr tile_cache(new ImageTileCache(tile_provider));
  tile_cache->set_weak_ptr(tile_cache);
  return tile_cache;
}

void
ImageTileCache::set_weak_ptr(ImageTileCachePtr self)
{
  m_self = self;
}

ImageTileCache::ImageTileCache(TileProviderPtr tile_provider) :
  m_self(),
  m_cache(),
  m_jobs(),
  m_tile_queue(),
  m_tile_provider(tile_provider),
  m_max_scale(m_tile_provider->get_max_scale()),
  m_min_keep_scale(m_max_scale - 2)
{
}

SurfacePtr
ImageTileCache::get_tile(int x, int y, int scale)
{
  if (x < 0 || y < 0 || scale < 0)
  {
    return SurfacePtr();
  }
  else
  {
    const unsigned int cache_id = make_cache_id(x, y, scale);
    Cache::iterator i = m_cache.find(cache_id);

    if (i != m_cache.end())
    {
      return i->second.surface;
    }
    else
    {
      return SurfacePtr();
    }
  }
}

ImageTileCache::SurfaceStruct
ImageTileCache::request_tile(int x, int y, int scale)
{
  const unsigned int cache_id = make_cache_id(x, y, scale);
  Cache::iterator i = m_cache.find(cache_id);

  if (i == m_cache.end())
  {
    m_jobs.push_back(m_tile_provider->request_tile(scale, Vector2i(x, y), 
                                                   weak(boost::bind(&ImageTileCache::receive_tile, _1, _2), m_self)));

    // FIXME: Something to try: Request the next smaller tile too,
    // so we get a lower quality image fast and a higher quality one
    // soon after FIXME: Its unclear if this actually improves
    // things, also the order of request gets mungled in the
    // DatabaseThread, we should request the whole group of lower
    // res tiles at once, instead of one by one, since that eats up
    // the possible speed up
    m_jobs.push_back(m_tile_provider->request_tile(scale, Vector2i(x, y), 
                                                   weak(boost::bind(&ImageTileCache::receive_tile, _1, _2), m_self)));
    SurfaceStruct s;
      
    s.surface = SurfacePtr();
    s.status  = SurfaceStruct::SURFACE_REQUESTED;

    m_cache[cache_id] = s;

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
  for(Jobs::iterator i = m_jobs.begin(); i != m_jobs.end(); ++i)
  {
    i->set_aborted();
  }
  m_jobs.clear();

  m_cache.clear();
}

void
ImageTileCache::cleanup()
{
  // FIXME: Cache cleanup should happen based on if the Tile is
  // visible, not if the image is visible

  // Image is not visible, so cancel all jobs
  for(Jobs::iterator i = m_jobs.begin(); i != m_jobs.end(); ++i)
  {
    i->set_aborted();
  }
  m_jobs.clear();
        
  // FIXME: We also need to purge the cache more often, since with
  // big gigapixel images we would end up never clearing it
      
  // Erase all tiles larger then 32x32

  // FIXME: Could make this more clever and relative to the number
  // of the images we display, since with large collections 32x32
  // might be to much for memory while with small collections it
  // will lead to unnecessary loading artifacts.      

  // FIXME: Code can hang here for some reason
  for(Cache::iterator i = m_cache.begin(); i != m_cache.end();)
  {
    const int tiledb_scale = (i->first >> 16);

    if (tiledb_scale < m_min_keep_scale ||
        i->second.status == SurfaceStruct::SURFACE_REQUESTED)
    {
      m_cache.erase(i++);
    }
    else
    {
      ++i;
    }
  }
}

SurfacePtr
ImageTileCache::find_smaller_tile(int x, int y, int tiledb_scale, int& downscale_out)
{
  int  downscale_factor = 1;

  while(downscale_factor < m_max_scale)
  {
    downscale_out = Math::pow2(downscale_factor);

    uint32_t cache_id = make_cache_id(x / downscale_out, y / downscale_out,
                                      tiledb_scale+downscale_factor);

    Cache::iterator i = m_cache.find(cache_id);
    if (i != m_cache.end() && i->second.surface)
    {
      return i->second.surface;
    }

    downscale_factor += 1;
  }

  return SurfacePtr();
}

void
ImageTileCache::process_queue()
{
  // Check the queue for newly arrived tiles
  while (!m_tile_queue.empty())
  {
    Tile tile = m_tile_queue.front();
    m_tile_queue.pop();
    
    int tile_id = make_cache_id(tile.get_pos().x, tile.get_pos().y, tile.get_scale());
  
    SurfaceStruct s;
  
    if (tile.get_surface())
    {
      s.surface = Surface::create(tile.get_surface());
      s.status  = SurfaceStruct::SURFACE_OK;
    }
    else
    {
      s.surface = SurfacePtr();
      s.status  = SurfaceStruct::SURFACE_FAILED;
    }

    m_cache[tile_id] = s;
  }
}

void
ImageTileCache::receive_tile(const Tile& tile)
{
  m_tile_queue.push(tile);

  Viewer::current()->redraw();
}

/* EOF */
