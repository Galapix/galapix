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
#include "database/row_id.hpp"
#include "database/database.hpp"
#include "plugins/jpeg.hpp"
#include "plugins/png.hpp"
#include "util/software_surface_factory.hpp"

TileDatabase::TileDatabase(SQLiteConnection& db, FileDatabase& files) :
  m_db(db),
  m_files(files),
  m_tile_table(m_db),
  m_tile_entry_store(m_db),
  m_tile_entry_get_all_by_fileid(m_db),
  m_tile_entry_has(m_db),
  m_tile_entry_get_by_fileid(m_db),
  m_tile_entry_get_min_max_scale(m_db),
  m_tile_entry_delete(m_db),
  m_cache()
{}

TileDatabase::~TileDatabase()
{
  flush_cache();
}

bool
TileDatabase::has_tile(const RowId& fileid, const Vector2i& pos, int scale)
{
  if (m_tile_entry_has(fileid, pos, scale))
  {
    return true;
  }
  else
  {
    return m_cache.has_tile(fileid, pos, scale);
  }
}

void
TileDatabase::get_tiles(const RowId& fileid, std::vector<TileEntry>& tiles_out)
{
  if (fileid)
  {
    m_tile_entry_get_all_by_fileid(fileid, tiles_out);
  }

  m_cache.get_tiles(fileid, tiles_out);
}

bool
TileDatabase::get_min_max_scale(const RowId& fileid, int& min_scale_out, int& max_scale_out)
{
  if (fileid)
  {
    if (m_tile_entry_get_min_max_scale(fileid, min_scale_out, max_scale_out))
    {
      int min_scale_out_cache = -1;
      int max_scale_out_cache = -1;

      if (m_cache.get_min_max_scale(fileid, min_scale_out_cache, max_scale_out_cache))
      {
        min_scale_out = std::min(min_scale_out, min_scale_out_cache);
        max_scale_out = std::max(max_scale_out, max_scale_out_cache);
      }

      return true;
    }
    else
    {
      return m_cache.get_min_max_scale(fileid, min_scale_out, max_scale_out);
    }
  }
  else
  {
    return m_cache.get_min_max_scale(fileid, min_scale_out, max_scale_out);
  }
}

bool
TileDatabase::get_tile(const RowId& fileid, int scale, const Vector2i& pos, TileEntry& tile_out)
{
  if (!fileid)
  {
    return m_cache.get_tile(fileid, scale, pos, tile_out);
  }
  else
  {
    if (m_tile_entry_get_by_fileid(fileid, scale, pos, tile_out))
    {
      return true;
    }
    else
    {
      return m_cache.get_tile(fileid, scale, pos, tile_out);
    }
  }
}

void
TileDatabase::store_tile(const RowId& fileid, const Tile& tile)
{
  if (!fileid)
  {
    std::cout << "Error: rejecting tile, fileid is not valid" << std::endl;
  }
  else
  {
    m_cache.store_tile(fileid, tile);

    // A single tile is ~10KB, but only in compressed JPEG form,
    // uncompressed tiles can be much bigger
    if (m_cache.size() > 256)
      flush_cache();
  }
}

void
TileDatabase::store_tiles(const std::vector<TileEntry>& tiles)
{
  m_db.exec("BEGIN;");
  for(std::vector<TileEntry>::const_iterator i = tiles.begin(); i != tiles.end(); ++i)
  {
    m_tile_entry_store(*i);
  }
  m_db.exec("END;");
}

void
TileDatabase::delete_tiles(const RowId& fileid)
{
  // FIXME: Ignoring cache
  m_tile_entry_delete(fileid);
}

void
TileDatabase::flush_cache()
{
  std::cout << "TileDatabes::flush_cache()" << std::endl;
#if 0
  m_files.flush_cache();
#endif
  m_cache.flush(*this);
}

/* EOF */
