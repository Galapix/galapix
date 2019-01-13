/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_TILE_ENTRY_GET_ALL_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_TILE_ENTRY_GET_ALL_HPP

class TileEntryGetAll final
{
public:
  TileEntryGetAll(SQLiteConnection& db) :
    m_stmt(db, "SELECT * FROM tiles ORDER BY image_id;")
  {}

  void operator()(std::vector<TileEntry>& tiles_out)
  {
  }

private:
  SQLiteStatement m_stmt;

private:
  TileEntryGetAll(const TileEntryGetAll&);
  TileEntryGetAll& operator=(const TileEntryGetAll&);
};

#endif

/* EOF */
