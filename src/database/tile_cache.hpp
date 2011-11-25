/*
**  Galapix - an image viewer for large image collections
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

#ifndef HEADER_GALAPIX_DATABASE_TILE_CACHE_HPP
#define HEADER_GALAPIX_DATABASE_TILE_CACHE_HPP

#include <vector>

#include "database/tile_entry.hpp"
#include "database/tile_database_interface.hpp"

class TileCache : public TileDatabaseInterface
{
private:
  std::vector<TileEntry> m_cache;

public:
  TileCache();

  bool has_tile(const FileId& fileid, const Vector2i& pos, int scale);
  bool get_tile(const FileId& fileid, int scale, const Vector2i& pos, TileEntry& tile_out);
  void get_tiles(const FileId& fileid, std::vector<TileEntry>& tiles);
  bool get_min_max_scale(const FileId& fileid, int& min_scale_out, int& max_scale_out);

  void store_tile(const FileId& fileid, const Tile& tile);
  void store_tiles(const std::vector<TileEntry>& tiles);

  void delete_tiles(const FileId& fileid);

  int  size() const { return static_cast<int>(m_cache.size()); }
  void flush(TileDatabaseInterface& tile_database);
  void flush_cache();

private:
  TileCache(const TileCache&);
  TileCache& operator=(const TileCache&);
};

#endif

/* EOF */
