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
#include <iostream>
#include <stdio.h>

#include "math/math.hpp"
#include "plugins/curl.hpp"
#include "plugins/jpeg.hpp"

namespace {

int get_max_scale(const Size& size, int tilesize)
{
  int width = std::max(size.width, size.height);
  int i = 0;

  while(width > tilesize)
  {
    width /= 2;
    i += 1;
  }

  return i;
}

} // namespace

ZoomifyTileProvider::ZoomifyTileProvider(const std::string& basedir, const Size& size, int tilesize) :
  m_size(size),
  m_tilesize(tilesize),
  m_basedir(basedir),
  m_max_scale(::get_max_scale(size, tilesize)),
  m_info(m_max_scale+1)
{
  std::cout << "ZoomifyTileProvider(): " << m_basedir << " " 
            << m_size << " " << m_tilesize << " " << m_max_scale << std::endl;

  for(int i = m_max_scale; i >= 0; --i)
  {
    int previous_tiles_count = 0;
    for(int j = i; j <= m_max_scale; ++j)
    {
      previous_tiles_count += m_info[j].m_size.get_area();
    }

    Size size_in_tiles((m_size.width  / Math::pow2(i) - 1) / get_tilesize() + 1,
                       (m_size.height / Math::pow2(i) - 1) / get_tilesize() + 1);

    std::cout << i << " " << previous_tiles_count << " " << size_in_tiles << " - " << size_in_tiles.get_area() << std::endl;

    m_info[i] = Info(size_in_tiles, previous_tiles_count);
  }

  std::cout << "ZoomifyTileProvider: " << basedir << " " << m_size << " " << m_tilesize << " " << m_max_scale << std::endl;
}

boost::shared_ptr<ZoomifyTileProvider> 
ZoomifyTileProvider::create(const URL& url)
{
  std::string content = url.get_blob()->str();

  std::string url_str = url.str();
  std::string basedir = url_str.substr(7, url_str.size() - 26); // FIXME: use basedir() instead of raw string cutting
  Size size;
  int  tilesize;
  int  num_tiles;
  
  // FIXME: this isn't exactly a tolerant way to parse the xml file
  int ret = sscanf(content.c_str(),
                   "<IMAGE_PROPERTIES WIDTH=\"%d\" HEIGHT=\"%d\" NUMTILES=\"%d\" NUMIMAGES=\"1\" VERSION=\"1.8\" TILESIZE=\"%d\" />",
                   &size.width, &size.height, &num_tiles, &tilesize);
  if (ret != 4)
  {
    throw std::runtime_error("ZoomifyTileProvider::create: Couldn't parse ImageProperties.xml");
  }
  else
  {
    return boost::shared_ptr<ZoomifyTileProvider>(new ZoomifyTileProvider(basedir, size, tilesize));
  }
}

int
ZoomifyTileProvider::get_tile_group(int scale, const Vector2i& pos)
{
  int tilenum = (m_info[scale].m_size.width * pos.y + pos.x) + m_info[scale].m_previous_tiles_count;
  // a tilegroup has 256 tiles
  return tilenum / 256;
}

JobHandle
ZoomifyTileProvider::request_tile(int scale, const Vector2i& pos, 
                                  const boost::function<void (Tile)>& callback)
{
  int tile_group = get_tile_group(scale, pos);

  // construct the URL of the tile
  std::ostringstream out;
  out << m_basedir << "TileGroup" << tile_group << "/" 
      << (m_max_scale - scale) << "-" << pos.x << "-" << pos.y << ".jpg";

  // load the tile
  if (0)
  {
    // FIXME: Could/should do this in a separate thread for http://
    BlobHandle blob = CURLHandler::get_data(out.str());
    SoftwareSurfaceHandle surface = JPEG::load_from_mem(blob->get_data(), blob->size());
    callback(Tile(scale, pos, surface));
  }
  else
  {
    try 
    {
      SoftwareSurfaceHandle surface = JPEG::load_from_file(out.str());
      callback(Tile(scale, pos, surface));
    }
    catch(std::exception& err)
    {
      std::cout << "ZoomifyTileProvider: " << err.what() << std::endl;
    }
  }

  JobHandle job_handle;
  job_handle.set_finished();
  return job_handle;
}
  
/* EOF */
