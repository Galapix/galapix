/*
**  Windstille - A Sci-Fi Action-Adventure Game
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmx.de>
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

#include "galapix/zoomify_tile_provider.hpp"

#include <sstream>

#include "math/math.hpp"
#include "plugins/curl.hpp"
#include "plugins/jpeg.hpp"

ZoomifyTileProvider::ZoomifyTileProvider(const std::string& basedir, const Size& size, int max_scale) :
  m_size(size),
  m_basedir(basedir),
  m_max_scale(max_scale),
  m_info()
{
  m_info.resize(m_max_scale);
  for(int i = m_max_scale-1; i >= 0; ++i)
  {
    int previous_tiles_count = 0;
    for(int j = i; j < m_max_scale; ++j)
    {
      previous_tiles_count += m_info[j].m_size.width * m_info[j].m_size.height;
    }

    m_info[i] = Info(Size(m_size.width  / Math::pow2(i) / get_tilesize(),
                          m_size.height / Math::pow2(i) / get_tilesize()),
                     previous_tiles_count);
  }
}

boost::shared_ptr<ZoomifyTileProvider> 
ZoomifyTileProvider::create(const URL& url)
{
  // get XML and parse it
  Size        size;
  std::string basedir = "";
  int         max_scale = 11;

  return boost::shared_ptr<ZoomifyTileProvider>(new ZoomifyTileProvider(basedir, size, max_scale));
}

int
ZoomifyTileProvider::get_tile_group(int scale, const Vector2i& pos)
{
  return (m_info[scale].m_size.width * pos.y + pos.x) + m_info[scale].m_previous_tiles_count;
}

JobHandle
ZoomifyTileProvider::request_tile(int scale, const Vector2i& pos, 
                                  const boost::function<void (Tile)>& callback)
{
  int tile_group = get_tile_group(scale, pos);

  // construct the URL of the tile
  std::ostringstream out;
  out << m_basedir << "/TileGroup" << tile_group << "/" 
      << (m_max_scale - scale) << "-" << pos.x << "-" << pos.y << ".jpg";

  // load the tile
  // FIXME: Could/should do this in a separate thread for http://
  BlobHandle blob = CURLHandler::get_data(out.str());
  SoftwareSurfaceHandle surface = JPEG::load_from_mem(blob->get_data(), blob->size());

  callback(Tile(scale, pos, surface));

  JobHandle job_handle;
  job_handle.set_finished();
  return job_handle;
}
  
/* EOF */
