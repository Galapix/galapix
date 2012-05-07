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

#ifndef HEADER_GALAPIX_JOBS_TILE_GENERATION_JOB_HPP
#define HEADER_GALAPIX_JOBS_TILE_GENERATION_JOB_HPP

#include <functional>
#include <boost/signals.hpp>
#include <mutex>

#include "database/file_entry.hpp"
#include "galapix/tile.hpp"
#include "job/job.hpp"
#include "math/vector2i.hpp"
#include "util/software_surface_factory.hpp"

class TileGenerationJob : public Job
{
private:
  struct TileRequest
  {
    JobHandle job_handle;
    int       scale;
    Vector2i  pos;
    std::function<void (Tile)> callback;
    
    TileRequest(const JobHandle& job_handle_,
                int scale_, const Vector2i& pos_,
                const std::function<void (Tile)>& callback_) :
      job_handle(job_handle_),
      scale(scale_), pos(pos_),
      callback(callback_)
    {}
  };

  typedef std::vector<TileRequest> TileRequests;
  
private: 
  std::mutex m_state_mutex;

  enum { 
    kWaiting,
    kRunning,
    kAborted,
    kDone
  } m_state;

  URL       m_url;
  FileEntry m_file_entry;

  /** Only valid if state is kRunning or kDone */
  int       m_min_scale;
  int       m_max_scale;
  
  int       m_min_scale_in_db;
  int       m_max_scale_in_db;
  
  /** Regular TileRequests */
  TileRequests m_tile_requests;

  /** TileRequests that came in when the process was already running */
  TileRequests m_late_tile_requests;
  
  typedef std::vector<Tile> Tiles;
  Tiles m_tiles;

  boost::signal<void (const RowId&, const Tile&)> m_sig_tile_callback;

public:
  TileGenerationJob(const FileEntry& file_entry, int min_scale_in_db, int max_scale_in_db);
  ~TileGenerationJob();

  /** Request a tile to be generated, returns true if the request will
      be honored, false if the tile generation is already in progress
      and the request has to be discarded */
  bool request_tile(const JobHandle& job_handle, int scale, const Vector2i& pos,
                    const std::function<void (Tile)>& callback);
  void run();

  URL get_url() const { return m_url; }

  bool is_aborted();

  boost::signal<void (const RowId&, const Tile&)>& sig_tile_callback() { return m_sig_tile_callback; }

private:
  void process_tile(const Tile& tile);
};

#endif

/* EOF */
