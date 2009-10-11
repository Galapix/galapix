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

#ifndef HEADER_GALAPIX_DATABASE_TILE_ENTRY_HAS_STATEMENT_HPP
#define HEADER_GALAPIX_DATABASE_TILE_ENTRY_HAS_STATEMENT_HPP

class TileEntryHasStatement
{
private:
  SQLiteStatement m_stmt;

public:
  TileEntryHasStatement(SQLiteConnection& db) :
    m_stmt(db, "SELECT (rowid) FROM tiles WHERE fileid = ?1 AND scale = ?2 AND x = ?3 AND y = ?4;")
  {}

  bool operator()(const FileEntry& file_entry, const Vector2i& pos, int scale)
  {
    if (!file_entry.has_fileid())
    {
      return false;
    }
    else
    {
      m_stmt.bind_int64(1, file_entry.get_fileid());
      m_stmt.bind_int(2, scale);
      m_stmt.bind_int(3, pos.x);
      m_stmt.bind_int(4, pos.y);

      SQLiteReader reader = m_stmt.execute_query();

      if (reader.next())
      {
        return true;
      }  
      else
      {
        return false;
      }
    }
  }

private:
  TileEntryHasStatement(const TileEntryHasStatement&);
  TileEntryHasStatement& operator=(const TileEntryHasStatement&);
};

#endif

/* EOF */
