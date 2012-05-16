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

#ifndef HEADER_GALAPIX_DATABASE_CACHED_TILE_DATABASE_HPP
#define HEADER_GALAPIX_DATABASE_CACHED_TILE_DATABASE_HPP

#include <memory>

#include "database/tile_database_interface.hpp"

class MemoryTileDatabase;
class Database;

class CachedTileDatabase : public TileDatabaseInterface
{
private:
  std::unique_ptr<MemoryTileDatabase> m_tile_cache;
  std::unique_ptr<TileDatabaseInterface> m_tile_database;

public:
  CachedTileDatabase(std::unique_ptr<TileDatabaseInterface> tile_database);
  ~CachedTileDatabase();
  
  bool has_tile(const RowId& fileid, const Vector2i& pos, int scale) override;
  bool get_tile(const RowId& fileid, int scale, const Vector2i& pos, TileEntry& tile_out) override;
  void get_tiles(const RowId& fileid, std::vector<TileEntry>& tiles) override;
  bool get_min_max_scale(const RowId& fileid, int& min_scale_out, int& max_scale_out) override;

  void store_tile(const RowId& fileid, const Tile& tile) override;
  void store_tiles(const std::vector<TileEntry>& tiles) override;

  void delete_tiles(const RowId& fileid) override;
  void flush_cache();

private:
  CachedTileDatabase(const CachedTileDatabase&) = delete;
  CachedTileDatabase& operator=(const CachedTileDatabase&) = delete;
};

#endif

/* EOF */
