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

#include <sstream>
#include "SDL.h"
#include "png.hpp"
#include "jpeg.hpp"
#include "tile_entry.hpp"
#include "tile_database.hpp"

TileDatabase::TileDatabase(SQLiteConnection* db)
  : db(db),
    store_stmt(db),
    get_stmt(db),
    get_all_by_fileid_stmt(db),
    get_all_stmt(db),
    has_stmt(db)
{
  db->exec("CREATE TABLE IF NOT EXISTS tiles ("
           "fileid  INTEGER, " // link to to files.rowid
           "scale   INTEGER, " // zoom level
           "x       INTEGER, " // X position in tiles
           "y       INTEGER, " // Y position in tiles
           "data    BLOB,    " // the image data, JPEG
           "quality INTEGER, " // the quality of the tile (default: 0) FIXME: not used
           "format  INTEGER"   // format of the data (0: JPEG, 1: PNG)
           ");");

  db->exec("CREATE INDEX IF NOT EXISTS tiles_index ON tiles ( fileid, x, y, scale );");

  // FIXME: This is brute force and doesn't handle collisions
  store_stmt.prepare("INSERT into tiles (fileid, scale, x, y, data, quality, format) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7);");

  get_stmt.prepare("SELECT * FROM tiles WHERE fileid = ?1 AND scale = ?2 AND x = ?3 AND y = ?4;");
  get_all_by_fileid_stmt.prepare("SELECT * FROM tiles WHERE fileid = ?1;");
  has_stmt.prepare("SELECT (rowid) FROM tiles WHERE fileid = ?1 AND scale = ?2 AND x = ?3 AND y = ?4;");

  get_all_stmt.prepare("SELECT * FROM tiles ORDER BY fileid;");
}

bool
TileDatabase::has_tile(uint32_t fileid, const Vector2i& pos, int scale)
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

void
TileDatabase::get_tiles(uint32_t fileid, std::vector<TileEntry>& tiles)
{
  get_all_by_fileid_stmt.bind_int(1, fileid);

  SQLiteReader reader = get_all_by_fileid_stmt.execute_query();
  while(reader.next())
    {
      TileEntry tile(reader.get_int(0), // fileid
                     reader.get_int(1), // scale
                     Vector2i(reader.get_int (2),
                              reader.get_int (3)),
                     SoftwareSurface());
      
      // FIXME: TileEntry shouldn't contain a SoftwareSurface, but a
      // Blob, so we don't do encode/decode when doing a database
      // merge
      Blob blob = reader.get_blob(4);
      switch(reader.get_int(6)) // format
        {
          case SoftwareSurface::JPEG_FILEFORMAT:
            tile.set_software_surface(JPEG::load_from_mem(blob.get_data(), blob.size()));
            break;

          case SoftwareSurface::PNG_FILEFORMAT:
            tile.set_software_surface(PNG::load_from_mem(blob.get_data(), blob.size()));
            break;
        }

      tiles.push_back(tile);
    }
}

bool
TileDatabase::get_tile(uint32_t fileid, int scale, const Vector2i& pos, TileEntry& tile)
{
  get_stmt.bind_int(1, fileid);
  get_stmt.bind_int(2, scale);
  get_stmt.bind_int(3, pos.x);
  get_stmt.bind_int(4, pos.y);

  SQLiteReader reader = get_stmt.execute_query();

  if (reader.next())
    {
      tile = TileEntry(reader.get_int(0), // fileid
                       reader.get_int(1), // scale
                       Vector2i(reader.get_int(2), // pos
                                reader.get_int(3)),
                       SoftwareSurface());

      // FIXME: Do this in a DecoderThread
      Blob blob = reader.get_blob(4);
      switch(reader.get_int(6)) // format
        {
          case SoftwareSurface::JPEG_FILEFORMAT:
            tile.set_software_surface(JPEG::load_from_mem(blob.get_data(), blob.size()));
            break;

          case SoftwareSurface::PNG_FILEFORMAT:
            tile.set_software_surface(PNG::load_from_mem(blob.get_data(), blob.size()));
            break;
        }

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
  Blob blob;

  switch(tile.get_software_surface().get_format())
    {
      case SoftwareSurface::RGB_FORMAT:
        blob = JPEG::save(tile.get_software_surface(), 75);
        break;

      case SoftwareSurface::RGBA_FORMAT:
        blob = PNG::save(tile.get_software_surface());
        break;

      default:
        assert(!"TileDatabase::store_tile: Unhandled format");
        break;
    }

  // FIXME: We need to update a already existing record, instead of
  // just storing a duplicate
  store_stmt.bind_int (1, tile.get_fileid());
  store_stmt.bind_int (2, tile.get_scale());
  store_stmt.bind_int (3, tile.get_pos().x);
  store_stmt.bind_int (4, tile.get_pos().y);
  store_stmt.bind_blob(5, blob);
  store_stmt.bind_int (6, 0);
  store_stmt.bind_int (7, tile.get_software_surface().get_format());

  store_stmt.execute();
}

void
TileDatabase::check()
{
  SQLiteReader reader = get_all_stmt.execute_query();

  while(reader.next())
    {
      /*
        uint32_t fileid = reader.get_int(0);
        int scale  = reader.get_int(1);
        int x      = reader.get_int(2);
        int y      = reader.get_int(3);
        Blob blob  = reader.get_blob(4);
      */
    }
}
  
/* EOF */
