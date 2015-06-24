/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_JOBS_MULTIPLE_TILE_GENERATION_JOB_HPP
#define HEADER_GALAPIX_JOBS_MULTIPLE_TILE_GENERATION_JOB_HPP

#include <functional>

#include "jobs/tile_generator.hpp"
#include "job/job.hpp"

/**
 * Simple Job for generating a range of Tiles, it doesn't allow any
 * way to add late Jobs like TileGenerationJob does
 */
class MultipleTileGenerationJob : public Job
{
private:
  URL m_url;
  int m_min_scale_in_db;
  int m_max_scale_in_db;
  int m_min_scale;
  int m_max_scale;
  std::function<void (Tile)> m_callback;

public:
  MultipleTileGenerationJob(const JobHandle& job_handle,
                            const URL& url,
                            int min_scale_in_db, int max_scale_in_db,
                            int min_scale, int max_scale,
                            const std::function<void (Tile)>& callback) :
    Job(job_handle),
    m_url(url),
    m_min_scale_in_db(min_scale_in_db),
    m_max_scale_in_db(max_scale_in_db),
    m_min_scale(min_scale),
    m_max_scale(max_scale),
    m_callback(callback)
  {}

  void run()
  {
    if (m_min_scale_in_db != -1 &&
        m_max_scale_in_db != -1)
    {
      m_min_scale = std::min(m_min_scale, m_min_scale_in_db);
      m_max_scale = std::min(m_max_scale, m_min_scale_in_db-1);
    }

    if (m_min_scale_in_db == -1 || m_min_scale < m_min_scale_in_db) // range is non-empty
    {
      TileGenerator::generate(m_url,
                              m_min_scale, m_max_scale,
                              m_callback);
    }

    get_handle().set_finished();
  }

private:
  MultipleTileGenerationJob(const MultipleTileGenerationJob&);
  MultipleTileGenerationJob& operator=(const MultipleTileGenerationJob&);
};

#endif

/* EOF */
