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

#include "database/sqlite_tile_database.hpp"

#include <iostream>

#include <surf/software_surface_factory.hpp>
#include <surf/plugins/jpeg.hpp>
#include <surf/plugins/png.hpp>

#include "database/entries/tile_entry.hpp"
#include "database/row_id.hpp"
#include "database/database.hpp"

namespace galapix {

SQLiteTileDatabase::SQLiteTileDatabase(SQLite::Database& db, ResourceDatabase& files) :
  m_db(db),
  //m_files(files),
  m_tile_table(m_db),
  m_tile_entry_store(m_db),
  m_tile_entry_get_all_by_fileid(m_db),
  m_tile_entry_has(m_db),
  m_tile_entry_get_by_fileid(m_db),
  m_tile_entry_get_min_max_scale(m_db),
  m_tile_entry_delete(m_db)
{}

SQLiteTileDatabase::~SQLiteTileDatabase()
{
}

bool
SQLiteTileDatabase::has_tile(RowId const& image_id, Vector2i const& pos, int scale)
{
  return m_tile_entry_has(image_id, pos, scale);
}

void
SQLiteTileDatabase::get_tiles(RowId const& image_id, std::vector<TileEntry>& tiles_out)
{
  if (image_id)
  {
    m_tile_entry_get_all_by_fileid(image_id, tiles_out);
  }
}

bool
SQLiteTileDatabase::get_min_max_scale(RowId const& image_id, int& min_scale_out, int& max_scale_out)
{
  return m_tile_entry_get_min_max_scale(image_id, min_scale_out, max_scale_out);
}

bool
SQLiteTileDatabase::get_tile(RowId const& image_id, int scale, Vector2i const& pos, TileEntry& tile_out)
{
  return m_tile_entry_get_by_fileid(image_id, scale, pos, tile_out);
}

void
SQLiteTileDatabase::store_tile(RowId const& image_id, Tile const& tile)
{
  if (!image_id)
  {
    std::cout << "Error: rejecting tile, image_id is not valid" << std::endl;
  }
  else
  {
    m_tile_entry_store(TileEntry{image_id, tile.get_scale(), tile.get_pos(), tile.get_surface()});
  }
}

void
SQLiteTileDatabase::store_tiles(std::vector<TileEntry> const& tiles)
{
  m_db.exec("BEGIN;");
  for(std::vector<TileEntry>::const_iterator i = tiles.begin(); i != tiles.end(); ++i)
  {
    m_tile_entry_store(*i);
  }
  m_db.exec("END;");
}

void
SQLiteTileDatabase::delete_tiles(RowId const& image_id)
{
  m_tile_entry_delete(image_id);
}

} // namespace galapix

/* EOF */
