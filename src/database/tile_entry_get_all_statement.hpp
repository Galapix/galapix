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

#ifndef HEADER_GALAPIX_DATABASE_TILE_ENTRY_GET_ALL_STATEMENT_HPP
#define HEADER_GALAPIX_DATABASE_TILE_ENTRY_GET_ALL_STATEMENT_HPP

class TileEntryGetAllStatement
{
private:
  SQLiteStatement m_stmt;

public:
  TileEntryGetAllStatement(SQLiteConnection& db) :
    m_stmt(db, "SELECT * FROM tiles ORDER BY fileid;")
  {}

  void operator()(std::vector<TileEntry>& tiles_out)
  {
  }

private:
  TileEntryGetAllStatement(const TileEntryGetAllStatement&);
  TileEntryGetAllStatement& operator=(const TileEntryGetAllStatement&);
};

#endif

/* EOF */
