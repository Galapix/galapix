/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
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

#include <assert.h>

#include "galapix/viewer.hpp"
#include "math/math.hpp"
#include "server/database_thread.hpp"
#include "util/weak_functor.hpp"

ImageTileCache::ImageTileCache(TileProviderPtr const& tile_provider) :
  m_cache(),
  m_tile_queue(),
  m_tile_provider(tile_provider),
  m_max_scale(m_tile_provider->get_max_scale()),
  m_min_keep_scale(m_max_scale - 2)
{
}

Surface
ImageTileCache::get_tile(int x, int y, int scale)
{
  if (x < 0 || y < 0 || scale < 0)
  {
    return {};
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
      return {};
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
    JobHandle job_handle = m_tile_provider->request_tile(
      scale, Vector2i(x, y),
      weak(std::mem_fn(&ImageTileCache::receive_tile), shared_from_this()));

    // FIXME: Something to try: Request the next smaller tile too,
    // so we get a lower quality image fast and a higher quality one
    // soon after FIXME: Its unclear if this actually improves
    // things, also the order of request gets mungled in the
    // DatabaseThread, we should request the whole group of lower
    // res tiles at once, instead of one by one, since that eats up
    // the possible speed up

    SurfaceStruct surface_struct(job_handle,
                                 SurfaceStruct::SURFACE_REQUESTED,
                                 Surface());
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
  for(Cache::iterator i = m_cache.begin(); i != m_cache.end();)
  {
    if (i->second.status == SurfaceStruct::SURFACE_REQUESTED)
    {
      i->second.job_handle.set_aborted();
      m_cache.erase(i++);
    }
    else if (i->second.status == SurfaceStruct::SURFACE_SUCCEEDED &&
             i->first.get_scale() < m_min_keep_scale)
    {
      m_cache.erase(i++);
    }
    else
    {
      ++i;
    }
  }
}

Surface
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

  return {};
}

void
ImageTileCache::process_queue()
{
  // Check the queue for newly arrived tiles
  Tile tile;
  while (m_tile_queue.try_pop(tile))
  {
    TileCacheId tile_id(tile.get_pos(), tile.get_scale());

    Cache::iterator i = m_cache.find(tile_id);

    if (i == m_cache.end())
    {
      // std::cout << "ImageTileCache::process_queue(): received unrequested tile" << std::endl;
      m_cache[tile_id] = SurfaceStruct(JobHandle::create(),
                                       SurfaceStruct::SURFACE_SUCCEEDED,
                                       Surface::create(tile.get_surface()));
    }
    else
    {
      i->second.surface = Surface::create(tile.get_surface());
      i->second.status = SurfaceStruct::SURFACE_SUCCEEDED;
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
    for(Cache::iterator i = m_cache.begin(); i != m_cache.end();)
    {
      if (i->second.status == SurfaceStruct::SURFACE_REQUESTED &&
          (scale != i->first.get_scale() || !rect.contains(i->first.get_pos())))
      {
        i->second.job_handle.set_aborted();
        m_cache.erase(i++);
      }
      else
      {
        ++i;
      }
    }
  }
}

void
ImageTileCache::receive_tile(const Tile& tile)
{
  m_tile_queue.wait_and_push(tile);

  Viewer::current()->redraw();
}

/* EOF */
