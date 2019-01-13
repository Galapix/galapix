/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2010 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_DATABASE_FILE_TILE_DATABASE_HPP
#define HEADER_GALAPIX_DATABASE_FILE_TILE_DATABASE_HPP

#include "database/tile_database_interface.hpp"

#include <string>

class FileTileDatabase : public TileDatabaseInterface
{
public:
  FileTileDatabase(const std::string& prefix);
  ~FileTileDatabase() override;

  bool has_tile(const RowId& image_id, const Vector2i& pos, int scale) override;
  bool get_tile(const RowId& image_id, int scale, const Vector2i& pos, TileEntry& tile_out) override;
  void get_tiles(const RowId& image_id, std::vector<TileEntry>& tiles) override;
  bool get_min_max_scale(const RowId& image_id, int& min_scale_out, int& max_scale_out) override;

  void store_tile(const RowId& image_id, const Tile& tile) override;
  void store_tiles(const std::vector<TileEntry>& tiles) override;

  void delete_tiles(const RowId& image_id) override;

  void check() {}

  void flush_cache() {}

private:
  std::string get_directory(const RowId& file_id);
  std::string get_filename(const Vector2i& pos, int scale);

  std::string get_complete_filename(const RowId& image_id, const Vector2i& pos, int scale);
  std::string get_complete_directory(const RowId& file_id);

  bool parse_filename(const std::string& filename, Vector2i* pos_out, int* scale_out, int* format);
  void ensure_directory_exists(const RowId& file_id);

private:
  std::string m_prefix;

private:
  FileTileDatabase(const FileTileDatabase&);
  FileTileDatabase& operator=(const FileTileDatabase&);
};

#endif

/* EOF */
