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

class MemoryTileDatabase : public TileDatabaseInterface
{
private:
  std::vector<TileEntry> m_cache;

public:
  MemoryTileDatabase();

  bool has_tile(const RowId& fileid, const Vector2i& pos, int scale) override;
  bool get_tile(const RowId& fileid, int scale, const Vector2i& pos, TileEntry& tile_out) override;
  void get_tiles(const RowId& fileid, std::vector<TileEntry>& tiles) override;
  bool get_min_max_scale(const RowId& fileid, int& min_scale_out, int& max_scale_out) override;

  void store_tile(const RowId& fileid, const Tile& tile) override;
  void store_tiles(const std::vector<TileEntry>& tiles) override;

  void delete_tiles(const RowId& fileid) override;

  int  size() const { return static_cast<int>(m_cache.size()); }
  void flush(TileDatabaseInterface& tile_database);
  void flush_cache() override;

private:
  MemoryTileDatabase(const MemoryTileDatabase&) = delete;
  MemoryTileDatabase& operator=(const MemoryTileDatabase&) = delete;
};

#endif

/* EOF */
