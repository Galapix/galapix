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
#include "database/tiles_table.hpp"
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

class TileDatabase : public TileDatabaseInterface
{
private:
  SQLiteConnection& m_db;
  FileDatabase& m_files;

  TilesTable m_tiles_table;
  TileEntryStoreStatement             m_tile_entry_store;
  TileEntryGetAllByFileEntryStatement m_tile_entry_get_all_by_fileid;
  TileEntryHasStatement               m_tile_entry_has;
  TileEntryGetByFileEntryStatement    m_tile_entry_get_by_fileid;
  TileEntryGetMinMaxScaleStatement    m_tile_entry_get_min_max_scale;
  TileEntryDeleteStatement            m_tile_entry_delete;
  
  TileCache m_cache;

public:
  TileDatabase(SQLiteConnection& db, FileDatabase& files);
  ~TileDatabase();
  
  bool has_tile(const FileId& fileid, const Vector2i& pos, int scale);
  bool get_tile(const FileId& fileid, int scale, const Vector2i& pos, TileEntry& tile_out);
  void get_tiles(const FileId& fileid, std::vector<TileEntry>& tiles);
  bool get_min_max_scale(const FileId& fileid, int& min_scale_out, int& max_scale_out);

  void store_tile(const FileId& fileid, const Tile& tile);
  void store_tiles(const std::vector<TileEntry>& tiles);

  void delete_tiles(const FileId& fileid);

  void flush_cache();

private:
  TileDatabase (const TileDatabase&);
  TileDatabase& operator= (const TileDatabase&);
};

#endif

/* EOF */
