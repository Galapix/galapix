/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmx.de>
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

#include "database/image_table.hpp"

ImageTable::ImageTable(SQLiteConnection& db) :
  m_db(db)
{
  m_db.exec("CREATE TABLE IF NOT EXISTS image (\n"
            "  id        INTEGER PRIMARY KEY AUTOINCREMENT,\n"
            "  file_id   INTEGER UNIQUE,\n"
            "  width     INTEGER,\n"
            "  height    INTEGER,\n"
            "  format    INTEGER\n" // format of the data (0: JPEG, 1: PNG)
            ");");

  m_db.exec("CREATE UNIQUE INDEX IF NOT EXISTS image_index ON image ( id, file_id, width, height, format );");
}

/* EOF */
