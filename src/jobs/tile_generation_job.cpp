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

#include "jobs/tile_generation_job.hpp"

#include <iostream>
#include <boost/bind.hpp>

#include "math/rect.hpp"
#include "plugins/jpeg.hpp"
#include "util/software_surface_factory.hpp"
#include "jobs/tile_generator.hpp"
#include "database/tile_entry.hpp"

TileGenerationJob::TileGenerationJob(const URL& url,
                                     const boost::function<void (FileEntry)>& file_callback,
                                     const boost::function<void (TileEntry)>& tile_callback) :
  m_state(kWaiting),
  m_file_entry(FileEntry::create_incomplete(url)),
  m_min_scale(-1),
  m_max_scale(-1),
  m_min_scale_in_db(-1),
  m_max_scale_in_db(-1),
  m_file_callback(file_callback),
  m_tile_callback(tile_callback),
  m_tile_requests(),
  m_late_tile_requests(),
  m_tiles() 
{
}

TileGenerationJob::TileGenerationJob(const FileEntry& file_entry, int min_scale_in_db, int max_scale_in_db,
                                     const boost::function<void (TileEntry)>& callback) :
  m_state(kWaiting),
  m_file_entry(file_entry),
  m_min_scale(-1),
  m_max_scale(-1),
  m_min_scale_in_db(min_scale_in_db),
  m_max_scale_in_db(max_scale_in_db),
  m_tile_callback(callback),
  m_tile_requests(),
  m_late_tile_requests(),
  m_tiles()
{
}

bool
TileGenerationJob::request_tile(const JobHandle& job_handle, int scale, const Vector2i& pos,
                                const boost::function<void (TileEntry)>& callback)
{
  boost::mutex::scoped_lock lock(m_state_mutex);
  
  switch(m_state)
  {
    case kWaiting:
      // We don't allow requests for tiles that are already in the database
      assert(scale < m_min_scale_in_db ||
             scale > m_max_scale_in_db);

      m_tile_requests.push_back(TileRequest(job_handle, scale, pos, callback));
      return true;

    case kRunning:
      if (m_min_scale <= scale && scale <= m_max_scale)
      {
        m_late_tile_requests.push_back(TileRequest(job_handle, scale, pos, callback));
        return true;
      }
      else
      {
        return false;
      }

    case kAborted:
      return false;

    case kDone:
      if (m_min_scale <= scale && scale <= m_max_scale)
      {
        Tiles::iterator tile_it = m_tiles.end();
        for(Tiles::iterator i = m_tiles.begin(); i != m_tiles.end(); ++i)
        {
          if (scale == i->get_scale() &&
              pos   == i->get_pos())
          {
            tile_it = i;
            break;
          }
        }

        if (tile_it != m_tiles.end())
        {
          callback(*tile_it);
          return true;
        }
        else
        {
          return false;
        }
      }
      else
      {
        return false;
      }

    default:
      assert(!"Never reached");
      return false;
  }
}

void
TileGenerationJob::process_tile_entry(const TileEntry& tile_entry)
{
  m_tiles.push_back(tile_entry);

  m_tile_callback(tile_entry);

  for(TileRequests::iterator i = m_tile_requests.begin(); i != m_tile_requests.end(); ++i)
  {
    if (!i->job_handle.is_aborted() &&
        i->pos   == tile_entry.get_pos() &&
        i->scale == tile_entry.get_scale())
    {
      i->callback(tile_entry);
    }
  }
}

void
TileGenerationJob::generate_tile_entries(SoftwareSurfaceFactory::FileFormat format,
                                         int min_scale, int max_scale)
{
  TileGenerator::generate(m_file_entry, format,
                          m_min_scale_in_db, m_max_scale_in_db,
                          min_scale, max_scale,
                          boost::bind(&TileGenerationJob::process_tile_entry, this, _1));
}

bool
TileGenerationJob::is_aborted()
{
  boost::mutex::scoped_lock lock(m_state_mutex);

  for(TileRequests::const_iterator i = m_tile_requests.begin(); i != m_tile_requests.end(); ++i)
  {
    if (!i->job_handle.is_aborted())
      return false;
  }

  m_state = kAborted;
 
  return true;
}

void
TileGenerationJob::run()
{
  if (!m_file_entry.is_complete())
  { // generate file entry
    Size size;
    if (!SoftwareSurfaceFactory::get_size(m_file_entry.get_url(), size))
    {
      std::cout << "TileGenerationJob::run(): Couldn't get size for " << m_file_entry.get_url() << std::endl;
      return;
    }
    else
    {
      m_file_entry.complete(size);
      m_file_callback(m_file_entry);
    }
  }

  SoftwareSurfaceFactory::FileFormat format;

  { // Calculate min/max_scale
    boost::mutex::scoped_lock lock(m_state_mutex);
    assert(m_state == kWaiting);
    m_state = kRunning;

    format = SoftwareSurfaceFactory::get_fileformat(m_file_entry.get_url());

    if (format != SoftwareSurfaceFactory::JPEG_FILEFORMAT)
    { 
      // Generate all tiles instead of just the requested for non-jpeg formats
      m_min_scale = 0;
      m_max_scale = m_file_entry.get_thumbnail_scale();
    }
    else
    {
      m_max_scale = (m_min_scale_in_db != -1) ? m_min_scale_in_db-1 : m_file_entry.get_thumbnail_scale();
      m_min_scale = m_max_scale;
    
      for(TileRequests::iterator i = m_tile_requests.begin(); i != m_tile_requests.end(); ++i)
      {
        m_min_scale = std::min(m_min_scale, i->scale);
      }
    }

    // catch weird database inconsisntencies
    if (m_min_scale == -1 || m_max_scale == -1)
    {
      std::cout << "[DEBUG] Database inconsisntencies: [" << m_min_scale << ".." << m_max_scale << "]" << std::endl;
      for(TileRequests::iterator i = m_tile_requests.begin(); i != m_tile_requests.end(); ++i)
      {
        std::cout << "[DEBUG] TileRequest: scale=" << i->scale << std::endl;
      }

      m_min_scale = 0;
      m_max_scale = m_file_entry.get_thumbnail_scale();
    }
  }

  try 
  {
    // Do the main work
    generate_tile_entries(format, m_min_scale, m_max_scale);
  }
  catch(const std::exception& err)
  {
    std::cout << "TileGenerationJob: Error while processing " << m_file_entry << std::endl;
    std::cout << "  Error: " << err.what() << std::endl;
  }

  {
    boost::mutex::scoped_lock lock(m_state_mutex);
    assert(m_state == kRunning);
    m_state = kDone;

    for(TileRequests::iterator j = m_late_tile_requests.begin(); j != m_late_tile_requests.end(); ++j)
    {
      for(Tiles::iterator i = m_tiles.begin(); i != m_tiles.end(); ++i)
      {
        if (j->scale == i->get_scale() &&
            j->pos   == i->get_pos())
        {
          j->callback(*i);
          break;
        }
      }
    }

    m_late_tile_requests.clear();
  }
}

/* EOF */
