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

#include "jobs/tile_generation_job.hpp"

#include <logmich/log.hpp>

#include "math/rect.hpp"
#include "plugins/jpeg.hpp"
#include "util/software_surface_factory.hpp"
#include "jobs/tile_generator.hpp"
#include "database/entries/tile_entry.hpp"

TileGenerationJob::TileGenerationJob(const OldFileEntry& file_entry, int min_scale_in_db, int max_scale_in_db) :
  Job(JobHandle::create()),
  m_state_mutex(),
  m_state(kWaiting),
  m_url(file_entry.get_url()),
  m_file_entry(file_entry),
  m_min_scale(-1),
  m_max_scale(-1),
  m_min_scale_in_db(min_scale_in_db),
  m_max_scale_in_db(max_scale_in_db),
  m_tile_requests(),
  m_late_tile_requests(),
  m_tiles(),
  m_sig_tile_callback()
{
}

TileGenerationJob::~TileGenerationJob()
{
  // FIXME: Verify that all JobHandles got finished
}

bool
TileGenerationJob::request_tile(const JobHandle& job_handle, int scale, const Vector2i& pos,
                                const std::function<void (Tile)>& callback)
{
  std::unique_lock<std::mutex> lock(m_state_mutex);

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
      assert(false && "Never reached");
      return false;
  }
}

void
TileGenerationJob::process_tile(const Tile& tile)
{
  m_tiles.push_back(tile);

  m_sig_tile_callback(m_file_entry.get_id(), tile);

  for(TileRequests::iterator i = m_tile_requests.begin(); i != m_tile_requests.end(); ++i)
  {
    if (!i->job_handle.is_aborted() &&
        i->pos   == tile.get_pos() &&
        i->scale == tile.get_scale())
    {
      i->callback(tile);
    }
  }
}

bool
TileGenerationJob::is_aborted()
{
#if 0
  std::unique_lock<std::mutex> lock(m_state_mutex);

  if (!m_file_entry)
  {
    if (get_handle().is_aborted())
    {
      m_state = kAborted;
      return true;
    }
    else
    {
      return false;
    }
  }
  else
  {
    for(TileRequests::const_iterator i = m_tile_requests.begin(); i != m_tile_requests.end(); ++i)
    {
      if (!i->job_handle.is_aborted())
        return false;
    }

    m_state = kAborted;
    return true;
  }
#else
  return false;
#endif
}

void
TileGenerationJob::run()
{
#if 0
  { // Calculate min/max_scale
    std::unique_lock<std::mutex> lock(m_state_mutex);
    assert(m_state == kWaiting);
    m_state = kRunning;

    if (m_url.is_remote() || !JPEG::filename_is_jpeg(m_url.str()))
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

      // catch weird database inconsisntencies
      if (m_min_scale == -1 || m_max_scale == -1)
      {
        log_error("[DEBUG] Database inconsisntencies: [%1%..%2%]", m_min_scale, m_max_scale);
        for(TileRequests::iterator i = m_tile_requests.begin(); i != m_tile_requests.end(); ++i)
        {
          log_error("[DEBUG] TileRequest: scale=%1%", i->scale);
        }

        m_min_scale = 0;
        m_max_scale = m_file_entry.get_thumbnail_scale();
      }
    }
  }

  try
  {
    // Do the main work
    TileGenerator::generate(m_url, m_min_scale, m_max_scale,
                            std::bind(&TileGenerationJob::process_tile, this, std::placeholders::_1));
  }
  catch(const std::exception& err)
  {
    log_error("Error while processing %1%", m_file_entry);
    log_error("  Exception: %1%", err.what());
  }

  {
    std::unique_lock<std::mutex> lock(m_state_mutex);
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
#endif
}

/* EOF */
