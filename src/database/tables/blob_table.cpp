/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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

#include "database/tables/blob_table.hpp"

#include "sqlite/connection.hpp"

BlobTable::BlobTable(SQLiteConnection& db) :
  m_db(db)
{
  m_db.exec("CREATE TABLE IF NOT EXISTS blob (\n"
            "  id        INTEGER PRIMARY KEY AUTOINCREMENT,\n"
            "  sha1      TEXT UNIQUE,\n"
            "  size      INTEGER\n"
            ");");

  //m_db.exec("CREATE INDEX IF NOT EXISTS blob_index ON blob ( sha1, size );");
}

/* EOF */
