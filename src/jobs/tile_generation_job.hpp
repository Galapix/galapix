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

#ifndef HEADER_GALAPIX_JOBS_TILE_GENERATION_JOB_HPP
#define HEADER_GALAPIX_JOBS_TILE_GENERATION_JOB_HPP

#include <functional>
#include <sigc++/signal.h>
#include <mutex>

#include <surf/software_surface_factory.hpp>

#include "database/entries/old_file_entry.hpp"
#include "galapix/tile.hpp"
#include "job/job.hpp"
#include "math/vector2i.hpp"

class TileGenerationJob : public Job
{
public:
  TileGenerationJob(const OldFileEntry& file_entry, int min_scale_in_db, int max_scale_in_db);
  ~TileGenerationJob() override;

  /** Request a tile to be generated, returns true if the request will
      be honored, false if the tile generation is already in progress
      and the request has to be discarded */
  bool request_tile(const JobHandle& job_handle, int scale, const Vector2i& pos,
                    const std::function<void (Tile)>& callback);
  void run() override;

  URL get_url() const { return m_url; }

  bool is_aborted() override;

  sigc::signal<void (const RowId&, const Tile&)>& sig_tile_callback() { return m_sig_tile_callback; }

private:
  void process_tile(const Tile& tile);

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

  using TileRequests = std::vector<TileRequest>;

private:
  std::mutex m_state_mutex;

  enum {
    kWaiting,
    kRunning,
    kAborted,
    kDone
  } m_state;

  URL m_url;
  OldFileEntry m_file_entry;

  /** Only valid if state is kRunning or kDone */
  int m_min_scale;
  int m_max_scale;

  int m_min_scale_in_db;
  int m_max_scale_in_db;

  /** Regular TileRequests */
  TileRequests m_tile_requests;

  /** TileRequests that came in when the process was already running */
  TileRequests m_late_tile_requests;

  using Tiles = std::vector<Tile>;
  Tiles m_tiles;

  sigc::signal<void (const RowId&, const Tile&)> m_sig_tile_callback;
};

#endif

/* EOF */
