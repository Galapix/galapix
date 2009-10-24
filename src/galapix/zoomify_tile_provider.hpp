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

#include "math/size.hpp"
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

    Info() :
      m_size(),
      m_previous_tiles_count()
    {}

    Info(const Size& size,
         int previous_tiles_count) :
      m_size(size),
      m_previous_tiles_count(previous_tiles_count)
    {}
  };

private:
  Size        m_size;
  std::string m_basedir;
  int         m_max_scale;
  std::vector<Info> m_info;

public:
  ZoomifyTileProvider(const std::string& basedir, const Size& size, int max_scale);

public:
  static boost::shared_ptr<ZoomifyTileProvider> create(const URL& url);

  int get_tile_group(int scale, const Vector2i& pos);
  JobHandle request_tile(int scale, const Vector2i& pos, 
                         const boost::function<void (Tile)>& callback);

  int  get_max_scale() const { return m_max_scale; }
  int  get_tilesize()  const { return 256; }
  int  get_overlap()   const { return 0; }
  Size get_size()      const { return m_size; }
  
private:
  ZoomifyTileProvider(const ZoomifyTileProvider&);
  ZoomifyTileProvider& operator=(const ZoomifyTileProvider&);
};

#endif

/* EOF */
