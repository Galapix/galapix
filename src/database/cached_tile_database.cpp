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

#include "database/memory_tile_database.hpp"
#include "database/database.hpp"

std::unique_ptr<MemoryTileDatabase> m_tile_cache;
std::unique_ptr<TileDatabaseInterface> m_tile_database;

CachedTileDatabase::CachedTileDatabase(std::unique_ptr<TileDatabaseInterface> tile_database) :
  m_tile_cache(std::make_unique<MemoryTileDatabase>()),
  m_tile_database(std::move(tile_database))
{
}

CachedTileDatabase::~CachedTileDatabase()
{
  flush_cache();
}

bool
CachedTileDatabase::has_tile(const RowId& image_id, const Vector2i& pos, int scale)
{
  return
    m_tile_cache->has_tile(image_id, pos, scale) ||
    m_tile_database->has_tile(image_id, pos, scale);
}

bool
CachedTileDatabase::get_tile(const RowId& image_id, int scale, const Vector2i& pos, TileEntry& tile_out)
{
  if (image_id)
  {
    if (m_tile_cache->get_tile(image_id, scale, pos, tile_out))
    {
      return true;
    }
    else
    {
      return m_tile_database->get_tile(image_id, scale, pos, tile_out);
    }
  }
  else
  {
    return false;
  }
}

void
CachedTileDatabase::get_tiles(const RowId& image_id, std::vector<TileEntry>& tiles)
{
  m_tile_cache->get_tiles(image_id, tiles);
  m_tile_database->get_tiles(image_id, tiles);
}

bool
CachedTileDatabase::get_min_max_scale(const RowId& image_id, int& min_scale_out, int& max_scale_out)
{
  if (image_id)
  {
    if (m_tile_cache->get_min_max_scale(image_id, min_scale_out, max_scale_out))
    {
      int min_scale = 0;
      int max_scale = 0;
      if (m_tile_database->get_min_max_scale(image_id, min_scale, max_scale))
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
CachedTileDatabase::store_tile(const RowId& image_id, const Tile& tile)
{
  if (!image_id)
  {
    m_tile_cache->store_tile(image_id, tile);
  }
  else
  {
    m_tile_database->store_tile(image_id, tile);
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
CachedTileDatabase::delete_tiles(const RowId& image_id)
{
  m_tile_cache->delete_tiles(image_id);
  m_tile_database->delete_tiles(image_id);
}

void
CachedTileDatabase::flush_cache()
{
  log_debug("CachedTileDatabase::flush_cache(): %1%", m_tile_cache->size());
  m_tile_cache->flush(*m_tile_database);
}

/* EOF */
