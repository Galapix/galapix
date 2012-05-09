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

#ifndef HEADER_GALAPIX_DATABASE_IMAGE_TABLE_HPP
#define HEADER_GALAPIX_DATABASE_IMAGE_TABLE_HPP

#include "sqlite/connection.hpp"

class ImageTable
{
private:
  SQLiteConnection& m_db;

public:
  ImageTable(SQLiteConnection& db) :
    m_db(db)
  {
    m_db.exec("CREATE TABLE IF NOT EXISTS image (\n"
              "  id        INTEGER PRIMARY KEY AUTOINCREMENT,\n"
              "  file_id   INTEGER UNIQUE,\n"
              "  width     INTEGER,\n"
              "  height    INTEGER,\n"
              "  handler   INTEGER\n"
              ");");

    m_db.exec("CREATE UNIQUE INDEX IF NOT EXISTS image_index ON image ( id, file_id, width, height, handler );");
  }

private:
  ImageTable(const ImageTable&) = delete;
  ImageTable& operator=(const ImageTable&) = delete;
};

#endif

/* EOF */
