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

#include <chrono>
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

namespace galapix {

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
                  wstdisplay::SurfacePtr surface_,
                  int attempts_ = 1) :
      job_handle(job_handle_),
      status(status_),
      surface(std::move(surface_)),
      attempts(attempts_)
    {}

    int attempts = 0;
  };

private:
  using Cache = std::map<TileCacheId, SurfaceStruct>;

public:
  ImageTileCache(TileProviderPtr const& tile_provider);

  /** Call after make_shared — must not run in the constructor (shared_from_this). */
  void prefetch_overview();

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
  void cancel_jobs(Rect const& rect, int scale);

  void receive_tile(Tile const& tile);

  int get_max_scale() const { return m_max_scale; }
  int get_min_scale() const { return m_min_scale; }

  /** Debounced scale for provider requests. Fast zoom holds the previous
      scale briefly so we do not enqueue a full grid at every intermediate
      log2 step (workers cannot cancel in-flight PDF/JPEG work). */
  int stable_request_scale(int desired_scale);

  static void set_tile_debug(bool on) { s_tile_debug = on; }
  static bool tile_debug() { return s_tile_debug; }

  /** Tiles with an outstanding provider job (SURFACE_REQUESTED). */
  int pending_request_count() const;

  /** Decoded tiles waiting for OpenGL upload (m_tile_queue). */
  int pending_upload_count() const;

  /** Entries currently held in the surface cache (any status). */
  int cache_entry_count() const { return static_cast<int>(m_cache.size()); }

  /** SUCCEEDED entries that have a GL surface ready to draw. */
  int ready_surface_count() const;

  /** Call once per Viewer::draw before images request tiles. Limits how many
      new provider jobs start this frame (global), to avoid stampeding thumtoo
      SQLite with 1000+ parallel get_tile calls on first paint. */
  static void begin_frame_request_budget(int max_new_requests);

  static bool try_consume_request_budget();

public:
  Cache m_cache;

  ThreadMessageQueue2<Tile> m_tile_queue;

  TileProviderPtr m_tile_provider;

  /** Coarsest scale (overview). */
  int m_max_scale;

  /** Finest scale the provider allows (0 for raster; may be negative for PDF). */
  int m_min_scale;

  /** Finest scale retained by cleanup(); set to min_scale to keep all. */
  int m_min_keep_scale;

  /** Last cancel_jobs args — skip O(cache) walk when unchanged. */
  bool m_have_last_cancel;
  int m_last_cancel_scale;
  Rect m_last_cancel_rect;

  /** Debounced request scale (see stable_request_scale). */
  bool m_have_stable_scale = false;
  int m_stable_scale = 0;
  int m_pending_scale = 0;
  std::chrono::steady_clock::time_point m_pending_since{};

private:
  /** Issue a provider request if this cell is not already in the cache.
      Does not recurse into stand-in requests (unlike request_tile). */
  void queue_tile_request(int x, int y, int scale);

  ImageTileCache(ImageTileCache const&);
  ImageTileCache& operator=(ImageTileCache const&);

  static bool s_tile_debug;
};

} // namespace galapix

#endif

/* EOF */
