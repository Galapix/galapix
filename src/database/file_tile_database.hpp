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

#ifndef HEADER_GALAPIX_DATABASE_FILE_TILE_DATABASE_INTERFACE_HPP
#define HEADER_GALAPIX_DATABASE_FILE_TILE_DATABASE_INTERFACE_HPP

#include "database/tile_database_interface.hpp"

#include <string>

class FileTileDatabase : public TileDatabaseInterface
{
private:
  std::string m_prefix;

public:
  FileTileDatabase(const std::string& prefix);
  ~FileTileDatabase();

  bool has_tile(const FileId& fileid, const Vector2i& pos, int scale);
  bool get_tile(const FileId& fileid, int scale, const Vector2i& pos, TileEntry& tile_out);
  void get_tiles(const FileId& fileid, std::vector<TileEntry>& tiles);
  bool get_min_max_scale(const FileId& fileid, int& min_scale_out, int& max_scale_out);

  void store_tile(const FileId& fileid, const Tile& tile);
  void store_tiles(const std::vector<TileEntry>& tiles);

  void delete_tiles(const FileId& fileid);

  void check() {}

  void flush_cache() {}

private:
  std::string get_directory(const FileId& file_id);
  std::string get_filename(const Vector2i& pos, int scale);

  std::string get_complete_filename(const FileId& fileid, const Vector2i& pos, int scale);
  std::string get_complete_directory(const FileId& file_id);

  bool parse_filename(const std::string& filename, Vector2i* pos_out, int* scale_out, int* format);
  void ensure_directory_exists(const FileId& file_id);

private:
  FileTileDatabase(const FileTileDatabase&);
  FileTileDatabase& operator=(const FileTileDatabase&);
};

#endif

/* EOF */
