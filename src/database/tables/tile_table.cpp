/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmail.com>
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

#include "database/tables/tile_table.hpp"

#include "sqlite/connection.hpp"

TileTable::TileTable(SQLiteConnection& db) :
  m_db(db)
{
  m_db.exec("CREATE TABLE IF NOT EXISTS tile (\n"
            "  image_id INTEGER,\n" // refers to images.imageid
            "  scale    INTEGER,\n" // zoom level
            "  x        INTEGER,\n" // X position in tiles
            "  y        INTEGER,\n" // Y position in tiles
            "  data     BLOB,\n"    // the image data, JPEG
            "  quality  INTEGER,\n" // the quality of the tile (default: 0) FIXME: not used
            "  format   INTEGER);"  // format of the data (0: JPEG, 1: PNG)
    );

  //m_db.exec("CREATE INDEX IF NOT EXISTS tile_index ON tile ( image_id, scale, x, y );");
}

/* EOF */
