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

#ifndef HEADER_GALAPIX_DATABASE_FILE_TILE_DATABASE_HPP
#define HEADER_GALAPIX_DATABASE_FILE_TILE_DATABASE_HPP

#include "database/tile_database_interface.hpp"

#include <string>

class FileTileDatabase : public TileDatabaseInterface
{
public:
  FileTileDatabase(std::string const& prefix);
  ~FileTileDatabase() override;

  bool has_tile(RowId const& image_id, Vector2i const& pos, int scale) override;
  bool get_tile(RowId const& image_id, int scale, Vector2i const& pos, TileEntry& tile_out) override;
  void get_tiles(RowId const& image_id, std::vector<TileEntry>& tiles) override;
  bool get_min_max_scale(RowId const& image_id, int& min_scale_out, int& max_scale_out) override;

  void store_tile(RowId const& image_id, Tile const& tile) override;
  void store_tiles(std::vector<TileEntry> const& tiles) override;

  void delete_tiles(RowId const& image_id) override;

  void check() {}

  void flush_cache() {}

private:
  std::string get_directory(RowId const& file_id);
  std::string get_filename(Vector2i const& pos, int scale);

  std::string get_complete_filename(RowId const& image_id, Vector2i const& pos, int scale);
  std::string get_complete_directory(RowId const& file_id);

  bool parse_filename(std::string const& filename, Vector2i* pos_out, int* scale_out, int* format);
  void ensure_directory_exists(RowId const& file_id);

private:
  std::string m_prefix;

private:
  FileTileDatabase(FileTileDatabase const&);
  FileTileDatabase& operator=(FileTileDatabase const&);
};

#endif

/* EOF */
