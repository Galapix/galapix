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

#ifndef HEADER_GALAPIX_GALAPIX_ZOOMIFY_TILE_PROVIDER_HPP
#define HEADER_GALAPIX_GALAPIX_ZOOMIFY_TILE_PROVIDER_HPP

#include "galapix/tile_provider.hpp"

class ZoomifyTileProvider : public TileProvider
{
private:
  struct Info
  {
    /** Size of the image in tiles */
    Size m_size;

    /** Number of tiles used by previous zoom levels */
    int m_previous_tiles_count;

    Info(const Size& size;
         int previous_tiles_count) :
      m_size(size),
      m_previous_tiles_count(previous_tiles_count)
    {}
  };

private:
  URL m_url;
  int m_max_scale;
  std::string m_basedir;
  std::vector<Info> m_info;

private:
  ZoomifyTileProvider(const URL& url) :
    m_url(url),
    m_max_scale(11), // FIXME: calculate max scale
    m_basedir(dirname(url.get_stdio_name()))
  {
    for(int i = 0; i < m_max_scale; ++i)
    {
      // FIXME: do some math
      m_info.push_back(Info());
    }
  }

public:
  static boost::shared_ptr<ZoomifTileProvider> create(const URL& url)
  {
    return boost::shared_ptr<ZoomifTileProvider>(new ZoomifTileProvider(url));
  }

  int get_tile_group(int scale, const Vector2i& pos)
  {
    int tilenum = (info[scale].m_size.width * pos.y + pos.x) + info[scale].m_previous_tiles_count
  }

  JobHandle request_tile(int scale, const Vector2i& pos, 
                         const boost::function<void (Tile)>& callback)
  {
    int tile_group = get_tile_group(scale, pos);

    // construct the URL of the tile
    std::ostringstream out;
    out << basedir << "/TileGroup" << tile_group << "/" 
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
  
  int get_max_scale() const 
  {
    return m_max_scale;
  }

private:
  ZoomifyTileProvider(const ZoomifyTileProvider&);
  ZoomifyTileProvider& operator=(const ZoomifyTileProvider&);
};

#endif

/* EOF */
