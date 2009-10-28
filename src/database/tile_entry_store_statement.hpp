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

#ifndef HEADER_GALAPIX_DATABASE_TILE_ENTRY_STORE_STATEMENT_HPP
#define HEADER_GALAPIX_DATABASE_TILE_ENTRY_STORE_STATEMENT_HPP

#include <iostream>

class TileEntryStoreStatement
{
private:
  SQLiteStatement m_stmt;

public:
  TileEntryStoreStatement(SQLiteConnection& db) :
    m_stmt(db, "INSERT into tiles (fileid, scale, x, y, data, quality, format) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7);")
    // FIXME: This is brute force and doesn't handle collisions
  {}

  void operator()(const TileEntry& tile_)
  {
    TileEntry tile = tile_;
  
    if (0)
      std::cout << "store_tile("
                << "fileid: " << tile.get_file_entry().get_fileid() 
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
          tile.set_format(TileEntry::JPEG_FORMAT);
          break;

        case SoftwareSurface::RGBA_FORMAT:
          tile.set_blob(PNG::save(tile.get_surface()));
          tile.set_format(TileEntry::PNG_FORMAT);
          break;

        default:
          assert(!"TileDatabase::store_tile: Unhandled format");
          break;
      }
    }

    // FIXME: We need to update a already existing record, instead of
    // just storing a duplicate
    m_stmt.bind_int64(1, tile.get_file_entry().get_fileid().get_id());
    m_stmt.bind_int (2, tile.get_scale());
    m_stmt.bind_int (3, tile.get_pos().x);
    m_stmt.bind_int (4, tile.get_pos().y);
    m_stmt.bind_blob(5, tile.get_blob());
    m_stmt.bind_int (6, 0);
    m_stmt.bind_int (7, tile.get_format());

    m_stmt.execute();
  }

private:
  TileEntryStoreStatement(const TileEntryStoreStatement&);
  TileEntryStoreStatement& operator=(const TileEntryStoreStatement&);
};

#endif

/* EOF */
