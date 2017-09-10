/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmail.com>
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

#include "database/tables/url_table.hpp"

#include "sqlite/connection.hpp"

URLTable::URLTable(SQLiteConnection& db) :
  m_db(db)
{
  m_db.exec("CREATE TABLE IF NOT EXISTS url (\n"
            "  id           INTEGER PRIMARY KEY AUTOINCREMENT,\n"
            "  url          TEXT,\n"
            "  content_type TEXT,\n"
            "  mtime        INTEGER,\n"
            "  blob_id      INTEGER\n"
            ");");

  //m_db.exec("CREATE UNIQUE INDEX IF NOT EXISTS url_index ON url ( id, blob_id, width, height, handler );");
}

/* EOF */