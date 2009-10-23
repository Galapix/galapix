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

#ifndef HEADER_GALAPIX_GALAPIX_IMAGE_TILE_CACHE_HPP
#define HEADER_GALAPIX_GALAPIX_IMAGE_TILE_CACHE_HPP

#include <map>
#include <vector>

#include "database/file_entry.hpp"
#include "database/tile_entry.hpp"
#include "display/surface.hpp"
#include "galapix/tile_provider.hpp"
#include "job/job_handle.hpp"
#include "job/thread_message_queue.hpp"

class ImageTileCache;

typedef boost::shared_ptr<ImageTileCache> ImageTileCacheHandle;

class ImageTileCache
{
public:
  struct SurfaceStruct 
  {
    enum Status 
    {
      SURFACE_OK,
      SURFACE_REQUESTED,
      SURFACE_FAILED 
    };
    
    Status  status;
    SurfaceHandle surface;

    SurfaceStruct() :
      status(),
      surface()
    {}
  };

private:
  typedef std::map<unsigned int, SurfaceStruct> Cache; 
  typedef std::vector<JobHandle> Jobs;

public:
  boost::weak_ptr<ImageTileCache> m_self;
  Cache m_cache;

  /** FIXME: Jobs array does not get cleared after jobs are done */
  Jobs m_jobs;

  ThreadMessageQueue<TileEntry> m_tile_queue;
  
  TileProviderHandle m_tile_provider;

  /** The maximum scale for which tiles exist */
  int m_max_scale;

  /** The smallest scale that is stored permanently */
  int m_min_keep_scale; 

private:
  ImageTileCache(TileProviderHandle tile_provider);

  void set_weak_ptr(ImageTileCacheHandle self);

public:
  static ImageTileCacheHandle create(TileProviderHandle tile_provider);

  SurfaceStruct request_tile(int x, int y, int scale);
  SurfaceHandle get_tile(int x, int y, int scale);
  SurfaceHandle find_smaller_tile(int x, int y, int tiledb_scale, int& downscale_out);

  void process_queue();

  /** Clear the cache completly */
  void clear();

  /** Cleanup the bigger tiles of the cache */
  void cleanup();

  void receive_tile(const TileEntry& tile);

  int get_max_scale() const { return m_max_scale; }

private:
  ImageTileCache(const ImageTileCache&);
  ImageTileCache& operator=(const ImageTileCache&);
};

#endif

/* EOF */
