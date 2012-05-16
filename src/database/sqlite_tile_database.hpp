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

#include "galapix/tile.hpp"
#include "database/file_tile_database.hpp"
#include "database/tile_table.hpp"
#include "database/tile_entry_get_all_by_file_entry_statement.hpp"
#include "database/tile_entry_has_statement.hpp"
#include "database/tile_entry_get_all_statement.hpp"
#include "database/tile_entry_store_statement.hpp"
#include "database/tile_entry_get_by_file_entry_statement.hpp"
#include "database/tile_entry_get_min_max_scale_statement.hpp"
#include "database/tile_entry_delete_statement.hpp"
#include "database/tile_cache.hpp"

class Database;
class FileDatabase;
class FileEntry;
class TileEntry;

class SQLiteTileDatabase : public TileDatabaseInterface
{
private:
  SQLiteConnection& m_db;
  FileDatabase& m_files;

  TileTable m_tile_table;
  TileEntryStoreStatement             m_tile_entry_store;
  TileEntryGetAllByFileEntryStatement m_tile_entry_get_all_by_fileid;
  TileEntryHasStatement               m_tile_entry_has;
  TileEntryGetByFileEntryStatement    m_tile_entry_get_by_fileid;
  TileEntryGetMinMaxScaleStatement    m_tile_entry_get_min_max_scale;
  TileEntryDeleteStatement            m_tile_entry_delete;
  
  TileCache m_cache;

public:
  SQLiteTileDatabase(SQLiteConnection& db, FileDatabase& files);
  ~SQLiteTileDatabase();
  
  bool has_tile(const RowId& fileid, const Vector2i& pos, int scale) override;
  bool get_tile(const RowId& fileid, int scale, const Vector2i& pos, TileEntry& tile_out) override;
  void get_tiles(const RowId& fileid, std::vector<TileEntry>& tiles) override;
  bool get_min_max_scale(const RowId& fileid, int& min_scale_out, int& max_scale_out) override;

  void store_tile(const RowId& fileid, const Tile& tile) override;
  void store_tiles(const std::vector<TileEntry>& tiles) override;

  void delete_tiles(const RowId& fileid) override;

  void flush_cache() override;

private:
  SQLiteTileDatabase (const SQLiteTileDatabase&) = delete;
  SQLiteTileDatabase& operator= (const SQLiteTileDatabase&) = delete;
};

#endif

/* EOF */
