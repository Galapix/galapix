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

#ifndef HEADER_GALAPIX_DATABASE_DATABASE_HPP
#define HEADER_GALAPIX_DATABASE_DATABASE_HPP

#include <memory>

#include "database/tile_database_interface.hpp"
#include "database/file_database.hpp"
#include "database/tile_cache.hpp"

/** */
class Database
{
private:
  std::unique_ptr<SQLiteConnection> m_db;
  std::unique_ptr<SQLiteConnection> m_tile_db;
  std::unique_ptr<FileDatabase> m_files;
  std::unique_ptr<TileDatabaseInterface> m_tiles;

public:
  Database(const std::string& prefix);
  ~Database();

  FileDatabase& get_files() { return *m_files; }
  TileDatabaseInterface& get_tiles() { return *m_tiles; }

  void delete_file_entry(const FileId& fileid);

  void cleanup();

private:
  Database (const Database&);
  Database& operator= (const Database&);
};

#endif

/* EOF */
