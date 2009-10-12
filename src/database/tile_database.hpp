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

#ifndef HEADER_GALAPIX_DATABASE_TILE_DATABASE_HPP
#define HEADER_GALAPIX_DATABASE_TILE_DATABASE_HPP

#include "sqlite/statement.hpp"
#include "math/vector2i.hpp"

#include "database/tiles_table.hpp"
#include "database/tile_entry_get_all_by_file_entry_statement.hpp"
#include "database/tile_entry_has_statement.hpp"
#include "database/tile_entry_get_all_statement.hpp"
#include "database/tile_entry_store_statement.hpp"
#include "database/tile_entry_get_by_file_entry_statement.hpp"
#include "database/tile_entry_get_min_max_scale_statement.hpp"
#include "database/tile_cache.hpp"

class TileEntry;
class FileEntry;
class Database;

class TileDatabase
{
private:
  Database& m_db;

  TilesTable m_tiles_table;
  TileEntryStoreStatement             m_tile_entry_store;
  TileEntryGetAllByFileEntryStatement m_tile_entry_get_all_by_file_entry;
  TileEntryHasStatement               m_tile_entry_has;
  TileEntryGetByFileEntryStatement    m_tile_entry_get_by_file_entry;
  TileEntryGetMinMaxScaleStatement    m_tile_entry_get_min_max_scale;

  TileCache m_cache;

public:
  TileDatabase(Database& db);
  ~TileDatabase();
  
  bool has_tile(const FileEntry& file_entry, const Vector2i& pos, int scale);
  bool get_tile(const FileEntry& file_entry, int scale, const Vector2i& pos, TileEntry& tile_out);
  void get_tiles(const FileEntry& file_entry, std::vector<TileEntry>& tiles);
  bool get_min_max_scale(const FileEntry& file_entry, int& min_scale_out, int& max_scale_out);

  void store_tile_in_cache(const TileEntry& tile);
  void store_tile(const TileEntry& tile);
  void store_tiles(const std::vector<TileEntry>& tiles);

  void check();

  void flush_cache();

private:
  TileDatabase (const TileDatabase&);
  TileDatabase& operator= (const TileDatabase&);
};

#endif

/* EOF */
