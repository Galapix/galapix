/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#include "database/database.hpp"

#include "database/file_tile_database.hpp"
#include "database/sqlite_tile_database.hpp"
#include "database/cached_tile_database.hpp"
#include "util/filesystem.hpp"

Database
Database::create(const std::string& prefix)
{
  Filesystem::mkdir(prefix);

  auto db = std::make_unique<SQLiteConnection>(prefix + "/cache4.sqlite3");
  auto tile_db = std::make_unique<SQLiteConnection>(prefix + "/cache4_tiles.sqlite3");

  auto resources = std::make_unique<ResourceDatabase>(*db);

  std::unique_ptr<CachedTileDatabase> tiles;
  if ((true))
  {
    tiles = std::make_unique<CachedTileDatabase>(std::make_unique<SQLiteTileDatabase>(*tile_db, *resources));
  }
  else
  {
    tiles = std::make_unique<CachedTileDatabase>(std::make_unique<FileTileDatabase>(prefix + "/tiles"));
  }

  return Database(std::move(db), std::move(tile_db), std::move(resources), std::move(tiles));
}

Database::Database(std::unique_ptr<SQLiteConnection>&& db,
                   std::unique_ptr<SQLiteConnection>&& tile_db,
                   std::unique_ptr<ResourceDatabase>&& resources,
                   std::unique_ptr<TileDatabaseInterface>&& tiles) :
  m_db(std::move(db)),
  m_tile_db(std::move(tile_db)),
  m_resources(std::move(resources)),
  m_tiles(std::move(tiles))
{
}

Database::~Database()
{
}

void
Database::delete_file_entry(const RowId& fileid)
{
  std::cout << "Begin Delete" << std::endl;
  m_db->exec("BEGIN;");
  m_tiles->delete_tiles(fileid);
  m_resources->delete_old_file_entry(fileid);
  m_db->exec("END;");
  std::cout << "End Delete" << std::endl;
}

void
Database::cleanup()
{
  m_db->vacuum();
}

/* EOF */
