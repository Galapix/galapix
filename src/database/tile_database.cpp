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
#include "database/database.hpp"
#include "plugins/jpeg.hpp"
#include "plugins/png.hpp"
#include "util/software_surface_factory.hpp"

TileDatabase::TileDatabase(Database& db)
  : m_db(db),
    m_tiles_table(m_db.get_db()),
    m_tile_entry_store(m_db.get_db()),
    m_tile_entry_get_all_by_file_entry(m_db.get_db()),
    m_tile_entry_has(m_db.get_db()),
    m_tile_entry_get_by_file_entry(m_db.get_db()),
    m_tile_entry_get_min_max_scale(m_db.get_db()),
    m_cache()
{}

TileDatabase::~TileDatabase()
{
  flush_cache();
}

bool
TileDatabase::has_tile(const FileEntry& file_entry, const Vector2i& pos, int scale)
{
  if (m_tile_entry_has(file_entry, pos, scale))
  {
    return true;
  }
  else
  {
    return m_cache.has_tile(file_entry, pos, scale);
  }
}

void
TileDatabase::get_tiles(const FileEntry& file_entry, std::vector<TileEntry>& tiles_out)
{
  if (file_entry.has_fileid())
  {
    m_tile_entry_get_all_by_file_entry(file_entry, tiles_out);
  }

  m_cache.get_tiles(file_entry, tiles_out);
}

bool
TileDatabase::get_min_max_scale(const FileEntry& file_entry, int& min_scale_out, int& max_scale_out)
{
  if (file_entry.has_fileid())
  {
    if (m_tile_entry_get_min_max_scale(file_entry, min_scale_out, max_scale_out))
    {
      int min_scale_out_cache = -1;
      int max_scale_out_cache = -1;

      if (m_cache.get_min_max_scale(file_entry, min_scale_out_cache, max_scale_out_cache))
      {
        min_scale_out = std::min(min_scale_out, min_scale_out_cache);
        max_scale_out = std::max(max_scale_out, max_scale_out_cache);
      }

      return true;
    }
    else
    {
      return m_cache.get_min_max_scale(file_entry, min_scale_out, max_scale_out);
    }
  }
  else
  {
    return m_cache.get_min_max_scale(file_entry, min_scale_out, max_scale_out);
  }
}

bool
TileDatabase::get_tile(const FileEntry& file_entry, int scale, const Vector2i& pos, TileEntry& tile_out)
{
  if (!file_entry.has_fileid())
  {
    return m_cache.get_tile(file_entry, scale, pos, tile_out);
  }
  else
  {
    if (m_tile_entry_get_by_file_entry(file_entry, scale, pos, tile_out))
    {
      return true;
    }
    else
    {
      return m_cache.get_tile(file_entry, scale, pos, tile_out);
    }
  }
}

void
TileDatabase::store_tile(const TileEntry& tile)
{
  m_cache.store_tile(tile);

  // A single tile is ~10KB
  if (m_cache.size() > 256)
    flush_cache();
}

void
TileDatabase::store_tiles(const std::vector<TileEntry>& tiles)
{
  m_db.get_db().exec("BEGIN;");
  for(std::vector<TileEntry>::const_iterator i = tiles.begin(); i != tiles.end(); ++i)
  {
    m_tile_entry_store(*i);
  }
  m_db.get_db().exec("COMMIT;");
}

void
TileDatabase::check()
{
#if 0
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
#endif
}

void
TileDatabase::flush_cache()
{
  std::cout << "TileDatabes::flush_cache()" << std::endl;
  m_db.files.flush_cache();
  m_cache.flush(*this);
}

/* EOF */
