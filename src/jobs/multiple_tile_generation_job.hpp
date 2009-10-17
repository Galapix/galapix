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

#ifndef HEADER_GALAPIX_JOBS_MULTIPLE_TILE_GENERATION_JOB_HPP
#define HEADER_GALAPIX_JOBS_MULTIPLE_TILE_GENERATION_JOB_HPP

#include <boost/function.hpp>

#include "jobs/tile_generator.hpp"
#include "job/job.hpp"

class MultipleTileGenerationJob : public Job
{
private:
  FileEntry m_file_entry;
  int m_min_scale_in_db;
  int m_max_scale_in_db;
  int m_min_scale; 
  int m_max_scale;
  boost::function<void (const TileEntry&)> m_callback;

public:
  MultipleTileGenerationJob(const JobHandle& job_handle, 
                            const FileEntry& file_entry,
                            int min_scale_in_db, int max_scale_in_db,
                            int min_scale, int max_scale,
                            const boost::function<void (const TileEntry&)>& callback) :
    Job(job_handle),
    m_file_entry(file_entry),
    m_min_scale_in_db(min_scale_in_db),
    m_max_scale_in_db(max_scale_in_db),
    m_min_scale(min_scale),
    m_max_scale(max_scale),
    m_callback(callback)
  {}

  void run()
  {
    SoftwareSurfaceFactory::FileFormat format = SoftwareSurfaceFactory::get_fileformat(m_file_entry.get_url());
    
    if (m_min_scale_in_db != -1 &&
        m_max_scale_in_db != -1)
    {
      m_min_scale = std::min(m_min_scale, m_min_scale_in_db);
      m_max_scale = std::min(m_max_scale, m_min_scale_in_db-1);
    }

    if (m_min_scale_in_db == -1 || m_min_scale < m_min_scale_in_db) // range is non-empty
    {
      TileGenerator::generate(m_file_entry, 
                              format,
                              m_min_scale_in_db, m_max_scale_in_db,
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