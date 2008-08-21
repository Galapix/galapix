/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#include <sstream>
#include "SDL.h"
#include "tile_entry.hpp"
#include "tile_database.hpp"

TileDatabase::TileDatabase(SQLiteConnection* db)
  : db(db),
    store_stmt(db),
    get_stmt(db),
    has_stmt(db)
{
  db->exec("CREATE TABLE IF NOT EXISTS tiles ("
           "fileid  INTEGER, " // link to to files.rowid
           "scale   INTEGER, " // zoom level
           "x       INTEGER, " // X position in tiles
           "y       INTEGER, " // Y position in tiles
           "data    BLOB     " // the image data, JPEG
           ");");

  db->exec("CREATE INDEX IF NOT EXISTS tiles_index ON tiles ( fileid, x, y, scale );");

  // FIXME: This is brute force and doesn't handle collisions
  store_stmt.prepare("INSERT into tiles (fileid, scale, x, y, data) VALUES (?1, ?2, ?3, ?4, ?5);");

  get_stmt.prepare("SELECT * FROM tiles WHERE fileid = ?1 AND scale = ?2 AND x = ?3 AND y = ?4;");
  has_stmt.prepare("SELECT (rowid) FROM tiles WHERE fileid = ?1 AND scale = ?2 AND x = ?3 AND y = ?4;");
}

bool
TileDatabase::has_tile(uint32_t fileid, Vector2i& pos, int scale)
{
  has_stmt.bind_int(1, fileid);
  has_stmt.bind_int(2, scale);
  has_stmt.bind_int(3, pos.x);
  has_stmt.bind_int(4, pos.y);

  SQLiteReader reader = has_stmt.execute_query();

  if (reader.next())
    {
      return true;
    }  
  else
    {
      return false;
    }
}

bool
TileDatabase::get_tile(uint32_t fileid, int scale, int x, int y, TileEntry& tile)
{
  //SDL_Delay(100);

  get_stmt.bind_int(1, fileid);
  get_stmt.bind_int(2, scale);
  get_stmt.bind_int(3, x);
  get_stmt.bind_int(4, y);

  SQLiteReader reader = get_stmt.execute_query();

  if (reader.next())
    {
      tile.fileid  = reader.get_int (0);
      tile.scale   = reader.get_int (1);
      tile.x       = reader.get_int (2);
      tile.y       = reader.get_int (3);

      // FIXME: Do this in the JPEGDecoderThread
      tile.surface = SoftwareSurface::from_data(reader.get_blob(4));

      return true;
    }
  else
    {
      // Tile missing

      return false;
    }
}

void
TileDatabase::store_tile(const TileEntry& tile)
{
  Blob blob = tile.surface.get_jpeg_data();

  // FIXME: We need to update a already existing record, instead of
  // just storing a duplicate
  store_stmt.bind_int (1, tile.fileid);
  store_stmt.bind_int (2, tile.scale);
  store_stmt.bind_int (3, tile.x);
  store_stmt.bind_int (4, tile.y);
  store_stmt.bind_blob(5, blob);

  store_stmt.execute();
}

void
TileDatabase::check()
{
  
}
  
/* EOF */
