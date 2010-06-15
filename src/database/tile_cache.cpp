/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmx.de>
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

#include "database/tile_cache.hpp"

#include "database/tile_database.hpp"

TileCache::TileCache() :
  m_cache()
{
}

bool
TileCache::has_tile(const FileEntry& file_entry, const Vector2i& pos, int scale)
{
  for(std::vector<TileEntry>::iterator i = m_cache.begin(); i != m_cache.end(); ++i)
  {
    if (i->get_file_entry() == file_entry &&
        i->get_scale()  == scale  &&
        i->get_pos()    == pos)
    {
      return true;
    }
  }

  return false;
}

bool
TileCache::get_tile(const FileEntry& file_entry, int scale, const Vector2i& pos, TileEntry& tile_out)
{
  for(std::vector<TileEntry>::iterator i = m_cache.begin(); i != m_cache.end(); ++i)
  {
    if (i->get_file_entry() == file_entry &&
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
TileCache::get_tiles(const FileEntry& file_entry, std::vector<TileEntry>& tiles_out)
{
  for(std::vector<TileEntry>::iterator i = m_cache.begin(); i != m_cache.end(); ++i)
  {
    if (i->get_file_entry() == file_entry)
    {
      tiles_out.push_back(*i);
    }
  }
}

bool
TileCache::get_min_max_scale(const FileEntry& file_entry, int& min_scale_out, int& max_scale_out)
{
  int min_scale = -1;
  int max_scale = -1;

  for(std::vector<TileEntry>::iterator i = m_cache.begin(); i != m_cache.end(); ++i)
  {
    if (i->get_file_entry() == file_entry)
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
TileCache::store_tile(const FileEntry& file_entry, const Tile& tile)
{
  m_cache.push_back(TileEntry(file_entry, tile.get_scale(), tile.get_pos(), tile.get_surface()));
}

void
TileCache::delete_tiles(const FileId& fileid)
{
  assert(!"not implemented");
}

void
TileCache::flush(TileDatabase& tile_database)
{
  if (!m_cache.empty())
  {
    tile_database.store_tiles(m_cache);
    m_cache.clear();
  }
}

void
TileCache::flush_cache()
{
}

/* EOF */
