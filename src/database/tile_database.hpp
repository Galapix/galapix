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

class TileEntry;
class FileEntry;

class TileDatabase
{
private:
  SQLiteConnection* db;
  SQLiteStatement store_stmt;
  SQLiteStatement get_stmt;
  SQLiteStatement get_all_by_fileid_stmt;
  SQLiteStatement get_all_stmt;
  SQLiteStatement has_stmt;

  std::vector<TileEntry> tile_cache;

public:
  TileDatabase(SQLiteConnection* db);
  ~TileDatabase();
  
  bool has_tile(const FileEntry& file_entry, const Vector2i& pos, int scale);
  bool get_tile(const FileEntry& file_entry, int scale, const Vector2i& pos, TileEntry& tile_out);
  void get_tiles(const FileEntry& file_entry, std::vector<TileEntry>& tiles);

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
