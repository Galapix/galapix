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

#include "database/database.hpp"

#include "database/file_tile_database.hpp"
#include "database/tile_database.hpp"
#include "database/cached_tile_database.hpp"
#include "util/filesystem.hpp"

Database::Database(const std::string& prefix) :
  m_db(),
  files(),
  tiles()
{
  Filesystem::mkdir(prefix);
  m_db.reset(new SQLiteConnection(prefix + "/cache3.sqlite3"));
  files.reset(new FileDatabase(*this));

  if (false)
  {
    tiles.reset(new TileDatabase(*this));
  }
  else
  {
    tiles.reset(new CachedTileDatabase(*this, new FileTileDatabase(prefix + "/tiles")));
  }
}

Database::~Database()
{
}

void
Database::cleanup()
{
  m_db->vacuum();
}

/* EOF */
