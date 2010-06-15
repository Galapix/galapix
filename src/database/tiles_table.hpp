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

#ifndef HEADER_GALAPIX_DATABASE_TILES_TABLE_HPP
#define HEADER_GALAPIX_DATABASE_TILES_TABLE_HPP

class TilesTable
{
private:
  SQLiteConnection& m_db;

public:
  TilesTable(SQLiteConnection& db) :
    m_db(db)
  {
    m_db.exec("CREATE TABLE IF NOT EXISTS tiles ("
               "fileid  INTEGER, " // refers to files.fileid
               "scale   INTEGER, " // zoom level
               "x       INTEGER, " // X position in tiles
               "y       INTEGER, " // Y position in tiles
               "data    BLOB,    " // the image data, JPEG
               "quality INTEGER, " // the quality of the tile (default: 0) FIXME: not used
               "format  INTEGER"   // format of the data (0: JPEG, 1: PNG)
               ");");

    m_db.exec("CREATE INDEX IF NOT EXISTS tiles_index ON tiles ( fileid );");
  }

private:
  TilesTable(const TilesTable&);
  TilesTable& operator=(const TilesTable&);
};

#endif

/* EOF */
