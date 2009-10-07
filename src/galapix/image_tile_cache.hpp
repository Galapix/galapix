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
#include "job/job_handle.hpp"
#include "job/thread_message_queue.hpp"

class ImageTileCache
{
private:
  struct SurfaceStruct 
  {
    enum Status 
    {
      SURFACE_OK,
      SURFACE_REQUESTED,
      SURFACE_FAILED 
    };
    
    Status  status;
    Surface surface;

    SurfaceStruct() :
      status(),
      surface()
    {}
  };

  typedef std::map<unsigned int, SurfaceStruct> Cache; 
  typedef std::vector<JobHandle> Jobs;

public:
  Cache cache;

  /** FIXME: Jobs array does not get cleared after jobs are done */
  Jobs jobs;

  ThreadMessageQueue<TileEntry> tile_queue;
  
  FileEntry file_entry; 

  /** The maximum scale for which tiles exist */
  int max_scale;

  /** The smallest scale that is stored permanently */
  int min_keep_scale; 

public:
  ImageTileCache(const FileEntry& file_entry);

  Surface request_tile(int x, int y, int scale);
  Surface get_tile(int x, int y, int scale);
  Surface find_smaller_tile(int x, int y, int tiledb_scale, int& downscale_out);

  void process_queue();

  /** Clear the cache completly */
  void clear();

  /** Cleanup the bigger tiles of the cache */
  void cleanup();

  void receive_tile(const TileEntry& tile);

  int get_max_scale() const { return max_scale; }

private:
  ImageTileCache(const ImageTileCache&);
  ImageTileCache& operator=(const ImageTileCache&);
};

#endif

/* EOF */
