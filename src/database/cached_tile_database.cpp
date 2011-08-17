/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2010 Ingo Ruhnke <grumbel@gmx.de>
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

#include "database/cached_tile_database.hpp"

#include <iostream>

#include "database/tile_cache.hpp"
#include "database/database.hpp"

std::unique_ptr<TileCache> m_tile_cache;
std::unique_ptr<TileDatabaseInterface> m_tile_database;

CachedTileDatabase::CachedTileDatabase(Database& db, TileDatabaseInterface* tile_database) :
  m_db(db),
  m_tile_cache(new TileCache),
  m_tile_database(tile_database)
{
}

CachedTileDatabase::~CachedTileDatabase()
{
  flush_cache();
}

bool
CachedTileDatabase::has_tile(const FileEntry& file_entry, const Vector2i& pos, int scale)
{
  return
    m_tile_cache->has_tile(file_entry, pos, scale) ||
    m_tile_database->has_tile(file_entry, pos, scale);
}

bool
CachedTileDatabase::get_tile(const FileEntry& file_entry, int scale, const Vector2i& pos, TileEntry& tile_out)
{
  if (file_entry.get_fileid())
  {
    if (m_tile_cache->get_tile(file_entry, scale, pos, tile_out))
    {
      return true;
    }
    else
    {
      return m_tile_database->get_tile(file_entry, scale, pos, tile_out);
    }
  }
  else
  {
    return false;
  }
}

void
CachedTileDatabase::get_tiles(const FileEntry& file_entry, std::vector<TileEntry>& tiles)
{
  m_tile_cache->get_tiles(file_entry, tiles);
  m_tile_database->get_tiles(file_entry, tiles);
}

bool
CachedTileDatabase::get_min_max_scale(const FileEntry& file_entry, int& min_scale_out, int& max_scale_out)
{
  if (file_entry.get_fileid())
  {
    if (m_tile_cache->get_min_max_scale(file_entry, min_scale_out, max_scale_out))
    {
      int min_scale = 0;
      int max_scale = 0;
      if (m_tile_database->get_min_max_scale(file_entry, min_scale, max_scale))
      {
        min_scale_out = std::min(min_scale_out, min_scale);
        max_scale_out = std::max(max_scale_out, max_scale);
        return true;
      }
      else
      {    
        return true;
      }
    }
    else
    {
      return false;
    }
  }
  else
  {
    return false;
  }
}

void
CachedTileDatabase::store_tile(const FileEntry& file_entry, const Tile& tile)
{
  if (!file_entry.get_fileid())
  {
    m_tile_cache->store_tile(file_entry, tile);
  }
  else
  {
    m_tile_database->store_tile(file_entry, tile);
  }

  if (m_tile_cache->size() > 256)
  {
    flush_cache();
  }
}

void
CachedTileDatabase::store_tiles(const std::vector<TileEntry>& tiles)
{
  m_tile_database->store_tiles(tiles);
}

void
CachedTileDatabase::delete_tiles(const FileId& fileid)
{
  m_tile_cache->delete_tiles(fileid);
  m_tile_database->delete_tiles(fileid);
}

void
CachedTileDatabase::flush_cache()
{
  std::cout << "CachedTileDatabase::flush_cache()" << std::endl;
  m_db.get_files().flush_cache();
  m_tile_cache->flush(*m_tile_database);
}

/* EOF */
