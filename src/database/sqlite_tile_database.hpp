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

#ifndef HEADER_GALAPIX_DATABASE_SQLITE_TILE_DATABASE_HPP
#define HEADER_GALAPIX_DATABASE_SQLITE_TILE_DATABASE_HPP

#include "sqlite/statement.hpp"
#include "math/vector2i.hpp"

#include "database/file_tile_database.hpp"
#include "database/memory_tile_database.hpp"
#include "database/statements/tile_entry_delete.hpp"
#include "database/statements/tile_entry_get_all_by_file_entry.hpp"
#include "database/statements/tile_entry_get_all.hpp"
#include "database/statements/tile_entry_get_by_file_entry.hpp"
#include "database/statements/tile_entry_get_min_max_scale.hpp"
#include "database/statements/tile_entry_has.hpp"
#include "database/statements/tile_entry_store.hpp"
#include "database/tables/tile_table.hpp"
#include "galapix/tile.hpp"

class Database;
class ResourceDatabase;
class FileEntry;
class TileEntry;

class SQLiteTileDatabase : public TileDatabaseInterface
{
private:
  SQLiteConnection& m_db;
  ResourceDatabase& m_files;

  TileTable m_tile_table;
  TileEntryStore             m_tile_entry_store;
  TileEntryGetAllByFileEntry m_tile_entry_get_all_by_fileid;
  TileEntryHas               m_tile_entry_has;
  TileEntryGetByFileEntry    m_tile_entry_get_by_fileid;
  TileEntryGetMinMaxScale    m_tile_entry_get_min_max_scale;
  TileEntryDelete            m_tile_entry_delete;

public:
  SQLiteTileDatabase(SQLiteConnection& db, ResourceDatabase& files);
  ~SQLiteTileDatabase();

  bool has_tile(const RowId& image_id, const Vector2i& pos, int scale) override;
  bool get_tile(const RowId& image_id, int scale, const Vector2i& pos, TileEntry& tile_out) override;
  void get_tiles(const RowId& image_id, std::vector<TileEntry>& tiles) override;
  bool get_min_max_scale(const RowId& image_id, int& min_scale_out, int& max_scale_out) override;

  void store_tile(const RowId& image_id, const Tile& tile) override;
  void store_tiles(const std::vector<TileEntry>& tiles) override;

  void delete_tiles(const RowId& image_id) override;

private:
  SQLiteTileDatabase (const SQLiteTileDatabase&) = delete;
  SQLiteTileDatabase& operator= (const SQLiteTileDatabase&) = delete;
};

#endif

/* EOF */
