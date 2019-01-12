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

#ifndef HEADER_GALAPIX_GALAPIX_ZOOMIFY_TILE_JOB_HPP
#define HEADER_GALAPIX_GALAPIX_ZOOMIFY_TILE_JOB_HPP

#include <iostream>

#include "util/url.hpp"
#include "math/vector2i.hpp"
#include "job/job.hpp"
#include "galapix/tile.hpp"
#include "network/curl.hpp"
#include "plugins/jpeg.hpp"

class ZoomifyTileJob : public Job
{
private:
  URL m_url;
  int m_scale;
  Vector2i m_pos;
  std::function<void (Tile)> m_callback;

public:
  ZoomifyTileJob(JobHandle job_handle, const URL& url,
                 int scale, const Vector2i& pos,
                 const std::function<void (Tile)>& callback) :
    Job(job_handle),
    m_url(url),
    m_scale(scale),
    m_pos(pos),
    m_callback(callback)
  {
  }

  void run() override
  {
    try
    {
      if (m_url.has_stdio_name())
      {
        SoftwareSurface surface = JPEG::load_from_file(m_url.get_stdio_name());
        m_callback(Tile(m_scale, m_pos, surface));
        get_handle().set_finished();
      }
      else
      {
        std::cout << "ZoomifyTileJob::run(): downloading " << m_url << std::endl;
        Blob blob = CURLHandler::get_data(m_url.str());
        SoftwareSurface surface = JPEG::load_from_mem(blob.get_data(), blob.size());
        m_callback(Tile(m_scale, m_pos, surface));
        get_handle().set_finished();
      }
    }
    catch(std::exception& err)
    {
      std::cout << "ZoomifyTileProvider: " << err.what() << std::endl;
      get_handle().set_failed();
    }
  }
};

#endif

/* EOF */
