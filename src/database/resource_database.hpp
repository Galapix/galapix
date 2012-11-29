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

#ifndef HEADER_GALAPIX_DATABASE_RESOURCE_DATABASE_HPP
#define HEADER_GALAPIX_DATABASE_RESOURCE_DATABASE_HPP

#include <vector>
#include <boost/optional.hpp>

#include "database/entries/resource_entry.hpp"
#include "database/statements/file_entry_delete.hpp"
#include "database/statements/file_entry_get_all.hpp"
#include "database/statements/file_entry_get_by_file_id.hpp"
#include "database/statements/file_entry_get_by_pattern.hpp"
#include "database/statements/file_entry_get_by_url.hpp"
#include "database/statements/file_entry_store.hpp"
#include "database/statements/image_entry_get.hpp"
#include "database/statements/image_entry_store.hpp"
#include "database/statements/resource_entry_get_by_blob_id.hpp"
#include "database/tables/archive_table.hpp"
#include "database/tables/blob_table.hpp"
#include "database/tables/file_table.hpp"
#include "database/tables/image_table.hpp"
#include "database/tables/video_table.hpp"
#include "math/size.hpp"
#include "sqlite/statement.hpp"

class Database;
class FileEntry;
class ImageEntry;
class TileEntry;
class URL;
class SHA1;

/** The ResourceDatabase keeps a record of all files that have been
    view. It keeps information on the last modification time and
    filesize to detect a need to regenerate the tiles and also handles
    the mapping from url to fileid, which is used for loookup of
    tiles in the TileDatabase. The ResourceDatabase also stores the size
    of an image, so that the image file itself doesn't need to be
    touched. */
class ResourceDatabase
{
private:
  SQLiteConnection& m_db;

  BlobTable                      m_blob_table;
  FileTable                      m_file_table;
  ImageTable                     m_image_table;
  ArchiveTable                   m_archive_table;
  VideoTable                     m_video_table;

  FileEntryGetAll          m_file_entry_get_all;
  FileEntryGetByFileId     m_file_entry_get_by_fileid;
  FileEntryGetByPattern    m_file_entry_get_by_pattern;
  FileEntryGetByUrl        m_file_entry_get_by_url;
  FileEntryStore           m_file_entry_store;
  FileEntryDelete          m_file_entry_delete;
  ImageEntryStore          m_image_entry_store;
  ImageEntryGet            m_image_entry_get;
  ResourceEntryGetByBlobId m_resource_entry_get_by_blob_id;

public:
  ResourceDatabase(SQLiteConnection& db);
  ~ResourceDatabase();
  
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
  boost::optional<ResourceEntry> get_resource_entry(const RowId& blob_id);
  void get_file_entries(std::vector<FileEntry>& entries_out);
  void get_file_entries(const std::string& pattern, std::vector<FileEntry>& entries_out);

  bool get_image_entry(const FileEntry& entry, ImageEntry& image_out);

  FileEntry  store_file_entry(const URL& url, int size, int mtime, FileEntry::Handler handler);
  FileEntry  store_file_entry(const URL& url, const SHA1& sha1, int size, int mtime, FileEntry::Handler handler, const RowId& archive_id);
  ImageEntry store_image_entry(const ImageEntry& image);

  void delete_file_entry(const RowId& file_id);

private:
  ResourceDatabase (const ResourceDatabase&);
  ResourceDatabase& operator= (const ResourceDatabase&);
};

#endif

/* EOF */
