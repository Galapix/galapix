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

#ifndef HEADER_FILE_DATABASE_HPP
#define HEADER_FILE_DATABASE_HPP

#include <stdint.h>
#include <sqlite3.h>
#include <string>

#include "sqlite.hpp"
#include "math/size.hpp"

class FileEntry;
class TileEntry;

/** The FileDatabase keeps a record of all files that have been
    view. It keeps information on the last modification time and
    filesize to detect a need to regenerate the tiles and also handles
    the mapping from filename to fileid, which is used for loookup of
    tiles in the TileDatabase. The FileDatabase also stores the size
    of an image, so that the image file itself doesn't need to be
    touched.
 */
class FileDatabase
{
private:
  SQLiteConnection* db;
  SQLiteStatement store_stmt;
  SQLiteStatement store_tile_stmt;
  SQLiteStatement get_by_filename_stmt;
  SQLiteStatement get_all_stmt;
  SQLiteStatement get_by_pattern_stmt;
  SQLiteStatement get_by_file_id_stmt;

  FileEntry store_file_entry(const std::string& filename, const Size& size);
  void delete_file_entry(uint32_t fileid);
  void update_file_entry(FileEntry& entry);
 
public:
  FileDatabase(SQLiteConnection* db);
  ~FileDatabase();
  
  /** Lookup a FileEntry by its filename. If there is no corresponding
      filename, then the file will be looked up in the filesystem and
      then stored in the DB and returned. If the file can't be found
      in either the DB or the filesystem false will be returned, else
      true
      
      @param[in] filename The absolute path of the file
      @param[out] entry   Lokation where the file information will be stored 
      @return true if lookup was successful, false otherwise, in which case entry stays untouched
  */
  FileEntry get_file_entry(const std::string& filename);
  void get_file_entries(std::vector<FileEntry>& entries);
  void get_file_entries(std::vector<FileEntry>& entries, const std::string& pattern);

  void store_tile(TileEntry& entry);

  void check();

private:
  FileDatabase (const FileDatabase&);
  FileDatabase& operator= (const FileDatabase&);
};

#endif

/* EOF */
