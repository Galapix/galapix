// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_GALAPIX_IMAGE_TILE_CACHE_HPP
#define HEADER_GALAPIX_GALAPIX_IMAGE_TILE_CACHE_HPP

#include <map>
#include <memory>
#include <vector>
#include <wstdisplay/surface.hpp>

#include "galapix/tile.hpp"
#include "galapix/tile_cache_id.hpp"
#include "galapix/tile_provider.hpp"
#include "job/job_handle.hpp"
#include "job/thread_message_queue2.hpp"
#include "math/rect.hpp"

// error: base class 'class std::enable_shared_from_this<>' has a non-virtual destructor
#pragma GCC diagnostic ignored "-Weffc++"

class ImageTileCache;

using ImageTileCachePtr = std::shared_ptr<ImageTileCache>;

class ImageTileCache : public std::enable_shared_from_this<ImageTileCache>
{
public:
  struct SurfaceStruct
  {
    enum Status
    {
      SURFACE_SUCCEEDED,
      SURFACE_REQUESTED
    };

    JobHandle job_handle;
    Status status;
    wstdisplay::SurfacePtr surface;

    SurfaceStruct() :
      job_handle(JobHandle::create()),
      status(),
      surface()
    {}

    SurfaceStruct(JobHandle const& job_handle_,
                  Status status_,
                  wstdisplay::SurfacePtr surface_) :
      job_handle(job_handle_),
      status(status_),
      surface(std::move(surface_))
    {}
  };

private:
  using Cache = std::map<TileCacheId, SurfaceStruct>;

public:
  ImageTileCache(TileProviderPtr const& tile_provider);

  SurfaceStruct request_tile(int x, int y, int scale);
  wstdisplay::SurfacePtr get_tile(int x, int y, int scale);
  wstdisplay::SurfacePtr find_smaller_tile(int x, int y, int tiledb_scale, int& downscale_out);

  void process_queue();

  /** Clear the cache completly */
  void clear();

  /** Cleanup the bigger tiles of the cache */
  void cleanup();

  /** \a rect and \a scale are the currently visible area, everything
      not in there will be canceled */
  void cancel_jobs(const Rect& rect, int scale);

  void receive_tile(const Tile& tile);

  int get_max_scale() const { return m_max_scale; }

public:
  Cache m_cache;

  ThreadMessageQueue2<Tile> m_tile_queue;

  TileProviderPtr m_tile_provider;

  /** The maximum scale for which tiles exist */
  int m_max_scale;

  /** The smallest scale that is stored permanently */
  int m_min_keep_scale;

private:
  ImageTileCache(const ImageTileCache&);
  ImageTileCache& operator=(const ImageTileCache&);
};

#endif

/* EOF */
