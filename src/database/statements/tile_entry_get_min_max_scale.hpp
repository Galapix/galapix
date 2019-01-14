// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_TILE_ENTRY_GET_MIN_MAX_SCALE_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_TILE_ENTRY_GET_MIN_MAX_SCALE_HPP

#include <assert.h>

class TileEntryGetMinMaxScale final
{
public:
  TileEntryGetMinMaxScale(SQLiteConnection& db) :
    m_stmt(db, "SELECT MIN(scale),MAX(scale) FROM tile WHERE image_id = ?1;")
  {}

  bool operator()(const RowId& fileid, int& min_scale_out, int& max_scale_out)
  {
    m_stmt.bind_int64(1, fileid.get_id());
    SQLiteReader reader = m_stmt.execute_query();

    if (reader.next())
    {
      if (reader.is_null(0) || reader.is_null(1))
      {
        // no tiles in the database
        return false;
      }
      else
      {
        min_scale_out = reader.get_int(0);
        max_scale_out = reader.get_int(1);
        return true;
      }
    }
    else
    {
      assert(false && "TileEntryGetMinMaxScale: never reached");
      return false;
    }
  }

private:
  SQLiteStatement m_stmt;

private:
  TileEntryGetMinMaxScale(const TileEntryGetMinMaxScale&);
  TileEntryGetMinMaxScale& operator=(const TileEntryGetMinMaxScale&);
};

#endif

/* EOF */
