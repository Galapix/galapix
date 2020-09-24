// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_JOBS_TILE_GENERATOR_HPP
#define HEADER_GALAPIX_JOBS_TILE_GENERATOR_HPP

#include <functional>

#include "surface/software_surface_factory.hpp"
#include "galapix/tile.hpp"

class FileEntry;

class TileGenerator
{
private:
  static const int TILE_WIDTH  = 256;
  static const int TILE_HEIGHT = 256;

public:
  static void generate_old(const URL& url,
                           int m_min_scale_in_db, int m_max_scale_in_db,
                           int min_scale, int max_scale,
                           const std::function<void (Tile)>& callback);

  static void generate(const URL& url, int min_scale, int max_scale,
                       const std::function<void(Tile)>& callback);

  static surf::SoftwareSurface load_surface(const URL& url, int min_scale, Size* size);

  /** Takes the given surface and cuts it into tiles which are then
      passed to callback. Surface can already be prescaled.
      min_scale/max_scale are the exact range for which tiles are
      generated. */
  static void cut_into_tiles(surf::SoftwareSurface surface,
                             const Size& original_size,
                             int min_scale, int max_scale,
                             const std::function<void (Tile)>& callback);

  /** Cuts the given surface into tiles, returning the results to the callback */
  static void cut_into_tiles(surf::SoftwareSurface const& surface,
                             const std::function<void (int x, int y, surf::SoftwareSurface)>& callback);

  /** Generate tiles for all the given downsample/scale levels */
  static void generate(surf::SoftwareSurface const& surface, int min_scale, int max_scale,
                       const std::function<void (int x, int y, int scale, surf::SoftwareSurface)>& callback);

private:
  TileGenerator(const TileGenerator&);
  TileGenerator& operator=(const TileGenerator&);
};

#endif

/* EOF */
