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

#include "database/tile_database.hpp"

#include <iostream>

#include "database/tile_entry.hpp"
#include "database/file_entry.hpp"
#include "plugins/jpeg.hpp"
#include "plugins/png.hpp"
#include "util/software_surface_factory.hpp"

TileDatabase::TileDatabase(SQLiteConnection* db_)
  : db(db_),
    store_stmt(db),
    get_stmt(db),
    get_all_by_fileid_stmt(db),
    get_all_stmt(db),
    has_stmt(db),
    tile_cache()
{
  db->exec("CREATE TABLE IF NOT EXISTS tiles ("
           "fileid  INTEGER, " // refers to files.fileid
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

TileDatabase::~TileDatabase()
{
  flush_cache();
}

bool
TileDatabase::has_tile(const FileEntry& file_entry, const Vector2i& pos, int scale)
{
  has_stmt.bind_int64(1, file_entry.get_fileid());
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
      // Check cache
      for(std::vector<TileEntry>::iterator i = tile_cache.begin(); i != tile_cache.end(); ++i)
        {
          if (i->get_fileid() == file_entry.get_fileid() &&
              i->get_scale()  == scale  &&
              i->get_pos()    == pos)
            {
              return true;
            }
        }

      return false;
    }
}

void
TileDatabase::get_tiles(const FileEntry& file_entry, std::vector<TileEntry>& tiles)
{
  get_all_by_fileid_stmt.bind_int64(1, file_entry.get_fileid());

  SQLiteReader reader = get_all_by_fileid_stmt.execute_query();
  while(reader.next())
    {
      TileEntry tile(file_entry,
                     reader.get_int(1), // scale
                     Vector2i(reader.get_int (2),  // x
                              reader.get_int (3)), // y
                     reader.get_blob(4),
                     reader.get_int(6));     
      
      // FIXME: TileEntry shouldn't contain a SoftwareSurface, but a
      // Blob, so we don't do encode/decode when doing a database
      // merge
      BlobHandle blob = tile.get_blob();
      switch(tile.get_format())
        {
          case SoftwareSurfaceFactory::JPEG_FILEFORMAT:
            tile.set_surface(JPEG::load_from_mem(blob->get_data(), blob->size()));
            break;

          case SoftwareSurfaceFactory::PNG_FILEFORMAT:
            tile.set_surface(PNG::load_from_mem(blob->get_data(), blob->size()));
            break;
        }

      tiles.push_back(tile);
    }

  // Check cache
  for(std::vector<TileEntry>::iterator i = tile_cache.begin(); i != tile_cache.end(); ++i)
    {
      if (i->get_fileid() == file_entry.get_fileid())
        {
          tiles.push_back(*i);
        }
    }
}

bool
TileDatabase::get_tile(const FileEntry& file_entry, int scale, const Vector2i& pos, TileEntry& tile)
{
  get_stmt.bind_int64(1, file_entry.get_fileid());
  get_stmt.bind_int(2, scale);
  get_stmt.bind_int(3, pos.x);
  get_stmt.bind_int(4, pos.y);

  SQLiteReader reader = get_stmt.execute_query();

  if (reader.next())
    {
      tile = TileEntry(file_entry,
                       reader.get_int(1), // scale
                       Vector2i(reader.get_int(2), // pos
                                reader.get_int(3)),
                       reader.get_blob(4),
                       reader.get_int(6));

      
      // FIXME: Do this in a DecoderThread

      BlobHandle blob = tile.get_blob();
      switch(tile.get_format())
        {
          case SoftwareSurfaceFactory::JPEG_FILEFORMAT:
            tile.set_surface(JPEG::load_from_mem(blob->get_data(), blob->size()));
            break;

          case SoftwareSurfaceFactory::PNG_FILEFORMAT:
            tile.set_surface(PNG::load_from_mem(blob->get_data(), blob->size()));
            break;
        }
      
      return true;
    }
  else
    {
      // Check cache
      for(std::vector<TileEntry>::iterator i = tile_cache.begin(); i != tile_cache.end(); ++i)
        {
          if (i->get_fileid() == file_entry.get_fileid() &&
              i->get_scale()  == scale  &&
              i->get_pos()    == pos)
            {
              tile = *i;
              return true;
            }
        }

      // Tile missing
      return false;
    }
}

void
TileDatabase::store_tile_in_cache(const TileEntry& tile)
{
  tile_cache.push_back(tile);

  // A single tile is ~10KB
  if (tile_cache.size() > 256)
    flush_cache();
}

void
TileDatabase::store_tiles(const std::vector<TileEntry>& tiles)
{
  db->exec("BEGIN;");
  for(std::vector<TileEntry>::const_iterator i = tiles.begin(); i != tiles.end(); ++i)
    {
      store_tile(*i);
    }
  db->exec("COMMIT;");
}

void
TileDatabase::store_tile(const TileEntry& tile_)
{
  TileEntry tile = tile_;
  
  if (0)
    std::cout << "store_tile("
              << "fileid: " << tile.get_fileid() 
              << ", scale: " << tile.get_scale() 
              << ", pos: " << tile.get_pos() << ")" << std::endl;

  if (!tile.get_blob())
    {
      // Tile doesn't have a Blob, so we assume it has a surface and
      // we generate the Blob from that
      switch(tile.get_surface()->get_format())
        {
          case SoftwareSurface::RGB_FORMAT:
            tile.set_blob(JPEG::save(tile.get_surface(), 75));
            tile.set_format(SoftwareSurfaceFactory::JPEG_FILEFORMAT);
            break;

          case SoftwareSurface::RGBA_FORMAT:
            tile.set_blob(PNG::save(tile.get_surface()));
            tile.set_format(SoftwareSurfaceFactory::PNG_FILEFORMAT);
            break;

          default:
            assert(!"TileDatabase::store_tile: Unhandled format");
            break;
        }
    }

  // FIXME: We need to update a already existing record, instead of
  // just storing a duplicate
  store_stmt.bind_int64(1, tile.get_fileid());
  store_stmt.bind_int (2, tile.get_scale());
  store_stmt.bind_int (3, tile.get_pos().x);
  store_stmt.bind_int (4, tile.get_pos().y);
  store_stmt.bind_blob(5, tile.get_blob());
  store_stmt.bind_int (6, 0);
  store_stmt.bind_int (7, tile.get_format());

  store_stmt.execute();
}

void
TileDatabase::check()
{
  SQLiteReader reader = get_all_stmt.execute_query();

  while(reader.next())
    {
      /*
        int64_t fileid = reader.get_int64(0);
        int scale  = reader.get_int(1);
        int x      = reader.get_int(2);
        int y      = reader.get_int(3);
        BlobHandle blob  = reader.get_blob(4);
      */
    }
}

void
TileDatabase::flush_cache()
{
  std::cout << "Flushing TileCache" << std::endl;

  if (!tile_cache.empty())
    {
      store_tiles(tile_cache);
      tile_cache.clear();
    }
}

/* EOF */
