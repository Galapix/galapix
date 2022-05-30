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

#include "database/memory_tile_database.hpp"

#include <algorithm>

#include "database/tile_database_interface.hpp"
#include "galapix/tile.hpp"

MemoryTileDatabase::MemoryTileDatabase() :
  m_cache()
{
}

bool
MemoryTileDatabase::has_tile(RowId const& image_id, Vector2i const& pos, int scale)
{
  for(std::vector<TileEntry>::iterator i = m_cache.begin(); i != m_cache.end(); ++i)
  {
    if (i->get_image_id() == image_id &&
        i->get_scale()  == scale  &&
        i->get_pos()    == pos)
    {
      return true;
    }
  }

  return false;
}

bool
MemoryTileDatabase::get_tile(RowId const& image_id, int scale, Vector2i const& pos, TileEntry& tile_out)
{
  for(std::vector<TileEntry>::iterator i = m_cache.begin(); i != m_cache.end(); ++i)
  {
    if (i->get_image_id() == image_id &&
        i->get_scale()  == scale  &&
        i->get_pos()    == pos)
    {
      tile_out = *i;
      return true;
    }
  }

  // Tile missing
  return false;
}

void
MemoryTileDatabase::get_tiles(RowId const& image_id, std::vector<TileEntry>& tiles_out)
{
  for(auto const& tile_entry : m_cache)
  {
    if (tile_entry.get_image_id() == image_id)
    {
      tiles_out.push_back(tile_entry);
    }
  }
}

bool
MemoryTileDatabase::get_min_max_scale(RowId const& image_id, int& min_scale_out, int& max_scale_out)
{
  int min_scale = -1;
  int max_scale = -1;

  for(std::vector<TileEntry>::iterator i = m_cache.begin(); i != m_cache.end(); ++i)
  {
    if (i->get_image_id() == image_id)
    {
      if (min_scale == -1)
      {
        min_scale = i->get_scale();
      }
      else
      {
        min_scale = std::min(min_scale, i->get_scale());
      }

      if (max_scale == -1)
      {
        max_scale = i->get_scale();
      }
      else
      {
        max_scale = std::max(max_scale, i->get_scale());
      }
    }
  }

  if (min_scale != -1 && max_scale != -1)
  {
    min_scale_out = min_scale;
    max_scale_out = max_scale;
    return true;
  }
  else
  {
    return false;
  }
}

void
MemoryTileDatabase::store_tile(RowId const& image_id, Tile const& tile)
{
  m_cache.push_back(TileEntry(image_id, tile.get_scale(), tile.get_pos(), tile.get_surface()));
}

void
MemoryTileDatabase::store_tiles(std::vector<TileEntry> const& tiles)
{
  m_cache.insert(m_cache.end(), tiles.begin(), tiles.end());
}

void
MemoryTileDatabase::delete_tiles(RowId const& image_id)
{
  m_cache.erase(std::remove_if(m_cache.begin(), m_cache.end(),
                               [&](TileEntry const& tile_entry) {
                                 return tile_entry.get_image_id() == image_id;
                               }),
                m_cache.end());
}

void
MemoryTileDatabase::flush(TileDatabaseInterface& tile_database)
{
  if (!m_cache.empty())
  {
    tile_database.store_tiles(m_cache);
    m_cache.clear();
  }
}

/* EOF */
