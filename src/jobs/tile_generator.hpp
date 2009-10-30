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

#ifndef HEADER_GALAPIX_JOBS_TILE_GENERATOR_HPP
#define HEADER_GALAPIX_JOBS_TILE_GENERATOR_HPP

#include <boost/function.hpp>

#include "util/software_surface_factory.hpp"
#include "galapix/tile.hpp"

class FileEntry;

class TileGenerator
{
private:
public:
  static void generate_old(const URL& url,
                           int m_min_scale_in_db, int m_max_scale_in_db,
                           int min_scale, int max_scale,
                           const boost::function<void (Tile)>& callback);

  static void generate(const URL& url, int min_scale, int max_scale,
                       const boost::function<void(Tile)>& callback);

  static SoftwareSurfacePtr load_surface(const URL& url, int min_scale, Size* size);

  static void cut_into_tiles(SoftwareSurfacePtr surface,
                             int min_scale, int max_scale,
                             const boost::function<void (Tile)>& callback);

private:
  TileGenerator(const TileGenerator&);
  TileGenerator& operator=(const TileGenerator&);
};

#endif

/* EOF */
