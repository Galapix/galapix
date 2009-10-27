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
    TileCacheId cache_id(Vector2i(x, y), scale);
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
  TileCacheId cache_id(Vector2i(x, y), scale);

  Cache::iterator i = m_cache.find(cache_id);

  if (i == m_cache.end())
  {
    JobHandle job_handle = m_tile_provider->request_tile(scale, Vector2i(x, y), 
                                                         weak(boost::bind(&ImageTileCache::receive_tile, _1, _2), m_self));

    // FIXME: Something to try: Request the next smaller tile too,
    // so we get a lower quality image fast and a higher quality one
    // soon after FIXME: Its unclear if this actually improves
    // things, also the order of request gets mungled in the
    // DatabaseThread, we should request the whole group of lower
    // res tiles at once, instead of one by one, since that eats up
    // the possible speed up

    SurfaceStruct surface_struct(job_handle,
                                 SurfaceStruct::SURFACE_REQUESTED,
                                 SurfacePtr());
    m_cache[cache_id] = surface_struct;
    return surface_struct;
  }
  else
  {
    return i->second;
  }
}

void
ImageTileCache::clear()
{
  for(Cache::iterator i = m_cache.begin(); i != m_cache.end(); ++i)
  {
    i->second.job_handle.set_aborted();
  }
  m_cache.clear();
}

void
ImageTileCache::cleanup()
{
  // Cancel all jobs and remove tiles smaller m_min_keep_scale
  for(Cache::iterator i = m_cache.begin(); i != m_cache.end(); ++i)
  {
    if (i->second.status == SurfaceStruct::SURFACE_REQUESTED)
    {
      i->second.job_handle.set_aborted();
      m_cache.erase(i);
    }
    else if (i->second.status == SurfaceStruct::SURFACE_SUCCEEDED &&
             i->first.get_scale() < m_min_keep_scale)
    {
      m_cache.erase(i);
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

    TileCacheId cache_id(Vector2i(x / downscale_out, y / downscale_out),
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
    
    TileCacheId tile_id(tile.get_pos(), tile.get_scale());
  
    Cache::iterator i = m_cache.find(tile_id);

    if (i == m_cache.end())
    {
      std::cout << "ImageTileCache::process_queue(): received unrequested tile" << std::endl;
    }
    else
    {
      if (tile.get_surface())
      {
        i->second.surface = Surface::create(tile.get_surface());
        i->second.status  = SurfaceStruct::SURFACE_SUCCEEDED;
      }
      else
      {
        i->second.surface = SurfacePtr();
        i->second.status  = SurfaceStruct::SURFACE_FAILED;
      }
    }
  }
}

struct TileReqestIsAborted
{
  bool operator()(const ImageTileCache::SurfaceStruct& request)
  {
    return request.job_handle.is_aborted();
  }
};

void
ImageTileCache::cancel_jobs(const Rect& rect, int scale)
{
  if (!m_cache.empty())
  {
    for(Cache::iterator i = m_cache.begin(); i != m_cache.end(); ++i)
    {
      if (i->second.status == SurfaceStruct::SURFACE_REQUESTED &&
          (scale != i->first.get_scale() || !rect.contains(i->first.get_pos())))
      {
        i->second.job_handle.set_aborted();              
        m_cache.erase(i);
      }
    }
  }
}

void
ImageTileCache::receive_tile(const Tile& tile)
{
  m_tile_queue.push(tile);

  Viewer::current()->redraw();
}

/* EOF */
