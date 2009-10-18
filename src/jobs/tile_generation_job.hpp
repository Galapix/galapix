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

#include <boost/function.hpp>
#include <boost/thread/mutex.hpp>

#include "database/file_entry.hpp"
#include "job/job.hpp"
#include "math/vector2i.hpp"
#include "util/software_surface_factory.hpp"

class TileEntry;

class TileGenerationJob : public Job
{
private:
  struct TileRequest
  {
    JobHandle job_handle;
    int       scale;
    Vector2i  pos;
    boost::function<void (TileEntry)> callback;
    
    TileRequest(const JobHandle& job_handle_,
                int scale_, const Vector2i& pos_,
                const boost::function<void (TileEntry)>& callback_) :
      job_handle(job_handle_),
      scale(scale_), pos(pos_),
      callback(callback_)
    {}
  };

  typedef std::vector<TileRequest> TileRequests;
  
private: 
  boost::mutex m_state_mutex;

  enum { 
    kWaiting,
    kRunning,
    kAborted,
    kDone
  } m_state;

  FileEntry m_file_entry;

  /** Only valid if state is kRunning or kDone */
  int       m_min_scale;
  int       m_max_scale;
  
  int       m_min_scale_in_db;
  int       m_max_scale_in_db;
  
  /** Callback for the FileEntry, only used when FileEntry isn't passed in the constructor*/
  boost::function<void (FileEntry)> m_file_callback;

  /** All generated tiles go through this callback */
  boost::function<void (TileEntry)> m_tile_callback;

  /** Regular TileRequests */
  TileRequests m_tile_requests;

  /** TileRequests that came in when the process was already running */
  TileRequests m_late_tile_requests;
  
  typedef std::vector<TileEntry> Tiles;
  Tiles m_tiles;

public:
  TileGenerationJob(const URL& url,
                    const boost::function<void (FileEntry)>& file_callback,
                    const boost::function<void (TileEntry)>& tile_callback);

  TileGenerationJob(const FileEntry& file_entry, int min_scale_in_db, int max_scale_in_db,
                    const boost::function<void (TileEntry)>& callback);

  /** Request a tile to be generated, returns true if the request will
      be honored, false if the tile generation is already in progress
      and the request has to be discarded */
  bool request_tile(const JobHandle& job_handle, int scale, const Vector2i& pos,
                    const boost::function<void (TileEntry)>& callback);
  void run();

  FileEntry get_file_entry() const { return m_file_entry; }

  bool is_aborted();

private:
  void generate_tile_entries(SoftwareSurfaceFactory::FileFormat format, 
                             int min_scale, int max_scale);
  void process_tile_entry(const TileEntry& tile_entry);
};

#endif

/* EOF */
