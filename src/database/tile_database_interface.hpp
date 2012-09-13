/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2010 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_DATABASE_TILE_DATABASE_INTERFACE_HPP
#define HEADER_GALAPIX_DATABASE_TILE_DATABASE_INTERFACE_HPP

#include <vector>

#include "math/vector2i.hpp"

class FileEntry;
class TileEntry;
class Tile;
class RowId;

class TileDatabaseInterface
{
public:
  TileDatabaseInterface() {}
  virtual ~TileDatabaseInterface() {}
  
  virtual bool has_tile(const RowId& fileid, const Vector2i& pos, int scale) =0;
  virtual bool get_tile(const RowId& fileid, int scale, const Vector2i& pos, TileEntry& tile_out) =0;
  virtual void get_tiles(const RowId& fileid, std::vector<TileEntry>& tiles) =0;
  virtual bool get_min_max_scale(const RowId& fileid, int& min_scale_out, int& max_scale_out) =0;

  virtual void store_tile(const RowId& fileid, const Tile& tile) =0;
  virtual void store_tiles(const std::vector<TileEntry>& tiles) =0;

  virtual void delete_tiles(const RowId& fileid) =0;

private:
  TileDatabaseInterface(const TileDatabaseInterface&);
  TileDatabaseInterface& operator=(const TileDatabaseInterface&);
};

#endif

/* EOF */
