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

#ifndef HEADER_GALAPIX_DATABASE_FILE_DATABASE_HPP
#define HEADER_GALAPIX_DATABASE_FILE_DATABASE_HPP

#include <vector>

#include "sqlite/statement.hpp"
#include "math/size.hpp"
#include "database/file_entry_delete_statement.hpp"
#include "database/file_entry_get_all_statement.hpp"
#include "database/file_entry_get_by_file_id_statement.hpp"
#include "database/file_entry_get_by_pattern_statement.hpp"
#include "database/file_entry_get_by_url_statement.hpp"
#include "database/file_entry_store_statement.hpp"
#include "database/image_entry_store_statement.hpp"
#include "database/archive_table.hpp"
#include "database/file_table.hpp"
#include "database/image_table.hpp"
#include "database/video_table.hpp"

class Database;
class FileEntry;
class ImageEntry;
class TileEntry;
class URL;

/** The FileDatabase keeps a record of all files that have been
    view. It keeps information on the last modification time and
    filesize to detect a need to regenerate the tiles and also handles
    the mapping from url to fileid, which is used for loookup of
    tiles in the TileDatabase. The FileDatabase also stores the size
    of an image, so that the image file itself doesn't need to be
    touched. */
class FileDatabase
{
private:
  SQLiteConnection& m_db;

  FileTable                      m_file_table;
  ImageTable                     m_image_table;
  ArchiveTable                   m_archive_table;
  VideoTable                     m_video_table;

  FileEntryGetAllStatement       m_file_entry_get_all;
  FileEntryGetByFileIdStatement  m_file_entry_get_by_fileid;
  FileEntryGetByPatternStatement m_file_entry_get_by_pattern;
  FileEntryGetByUrlStatement     m_file_entry_get_by_url;
  FileEntryStoreStatement        m_file_entry_store;
  FileEntryDeleteStatement       m_file_entry_delete;
  ImageEntryStoreStatement       m_image_entry_store;

public:
  FileDatabase(SQLiteConnection& db);
  ~FileDatabase();
  
  /**
     Lookup a FileEntry by its url. If there is no corresponding
     url, then the file will be looked up in the filesystem and
     then stored in the DB and returned. If the file can't be found
     in either the DB or the filesystem false will be returned, else
     true
      
     @param[in] url The absolute path of the file
     @param[out] entry   Lokation where the file information will be stored 
     @return true if lookup was successful, false otherwise, in which case entry stays untouched
  */
  bool get_file_entry(const URL& url, FileEntry& entry_out);
  void get_file_entries(std::vector<FileEntry>& entries_out);
  void get_file_entries(const std::string& pattern, std::vector<FileEntry>& entries_out);

  FileEntry store_file_entry(const URL& url, int size, int mtime, int type);
  void store_image_entry(const ImageEntry& image);

  void delete_file_entry(const RowId& fileid);

  void check();
  //void flush_cache();

private:
  FileDatabase (const FileDatabase&);
  FileDatabase& operator= (const FileDatabase&);
};

#endif

/* EOF */
