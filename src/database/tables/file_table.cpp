// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "database/tables/file_table.hpp"

#include <SQLiteCpp/Database.h>

namespace galapix {

FileTable::FileTable(SQLite::Database& db) :
  m_db(db)
{
  m_db.exec("CREATE TABLE IF NOT EXISTS file (\n"
            "  id       INTEGER PRIMARY KEY AUTOINCREMENT,\n"
            "  path     TEXT UNIQUE,\n"
            "  mtime    INTEGER,\n"
            "  blob_id  INTEGER\n"
            ");");

  //m_db.exec("CREATE INDEX IF NOT EXISTS file_index ON file ( url, mtime, handler, blob_id );");
}

} // namespace galapix

/* EOF */
