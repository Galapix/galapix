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

#include "database/resource_database.hpp"

#include <iostream>
#include <logmich/log.hpp>

#include "database/database.hpp"
#include "database/entries/old_file_entry.hpp"
#include "database/statements/blob_info_store.hpp"
#include "database/statements/file_info_get_by_path.hpp"
#include "database/statements/file_info_store.hpp"
#include "database/statements/image_info_get.hpp"
#include "database/statements/image_info_store.hpp"
#include "database/statements/resource_info_get.hpp"
#include "database/statements/resource_info_store.hpp"
#include "database/statements/url_info_get.hpp"
#include "database/statements/url_info_store.hpp"
#include "util/filesystem.hpp"
#include "util/software_surface.hpp"
#include "util/software_surface_factory.hpp"

ResourceDatabase::ResourceDatabase(SQLiteConnection& db) :
  m_db(db),

  m_archive_table(m_db),
  m_blob_table(m_db),
  m_file_table(m_db),
  m_image_table(m_db),
  m_resource_table(m_db),
  m_url_table(m_db),
  m_video_table(m_db),
  m_file_info_store(new FileInfoStore(m_db)),
  m_file_info_get_by_path(new FileInfoGetByPath(m_db)),
  m_image_info_store(new ImageInfoStore(m_db)),
  m_image_info_get(new ImageInfoGet(m_db)),
  m_url_info_store(new URLInfoStore(m_db)),
  m_url_info_get(new URLInfoGet(m_db)),
  m_resource_info_get(new ResourceInfoGet(m_db)),
  m_resource_info_store(new ResourceInfoStore(m_db)),
  m_blob_info_store(new BlobInfoStore(m_db)),
  m_file_entry_get_all(m_db),
  //m_file_entry_get_by_fileid(m_db),
  m_file_entry_get_by_pattern(m_db),
  m_file_entry_get_by_path(m_db),
  m_file_entry_store(m_db),
  m_file_entry_delete(m_db),
  m_image_entry_store(m_db),
  m_image_entry_get(m_db),
  m_resource_entry_get_by_blob_id(m_db)
{
}

ResourceDatabase::~ResourceDatabase()
{
}

boost::optional<ImageInfo>
ResourceDatabase::get_image_info(const ResourceInfo& resource)
{
  return (*m_image_info_get)(resource);
}

RowId
ResourceDatabase::store_image_info(const ImageInfo& image_info)
{
  return (*m_image_info_store)(image_info);
}

boost::optional<ResourceInfo>
ResourceDatabase::get_resource_info(const ResourceLocator& locator, const BlobInfo& blob)
{
  return (*m_resource_info_get)(locator, blob);
}

boost::optional<ResourceInfo>
ResourceDatabase::get_resource_info(const BlobInfo& blob)
{
  return (*m_resource_info_get)(blob);
}

RowId
ResourceDatabase::store_resource_info(const ResourceInfo& info)
{
  return (*m_resource_info_store)(info);
}

boost::optional<URLInfo>
ResourceDatabase::get_url_info(const std::string& url)
{
  return (*m_url_info_get)(url);
}

RowId
ResourceDatabase::store_url_info(const URLInfo& url_info)
{
  if (!url_info.get_blob_info().get_id())
  {
    (*m_blob_info_store)(url_info.get_blob_info());
  }
  return (*m_url_info_store)(url_info);
}

boost::optional<FileInfo>
ResourceDatabase::get_file_info(const std::string& path)
{
  return (*m_file_info_get_by_path)(path);
}

RowId
ResourceDatabase::store_file_info(const FileInfo& file_info)
{
  if (!file_info.get_blob_info().get_id())
  {
    (*m_blob_info_store)(file_info.get_blob_info());
  }
  return (*m_file_info_store)(file_info);
}

boost::optional<FileEntry>
ResourceDatabase::get_file_entry(const std::string& path)
{
  return m_file_entry_get_by_path(path);
}

void
ResourceDatabase::get_file_entries(std::vector<FileEntry>& entries_out)
{
  return m_file_entry_get_all(entries_out);
}

void
ResourceDatabase::get_file_entries(const std::string& pattern, std::vector<FileEntry>& entries_out)
{
  return m_file_entry_get_by_pattern(pattern, entries_out);
}

void
ResourceDatabase::store_file_entry(const std::string& path, int mtime)
{
  m_file_entry_store(path, mtime);
}

OldFileEntry
ResourceDatabase::store_old_file_entry(const URL& url, int size, int mtime, OldFileEntry::Handler handler)
{
  //RowId file_id = m_file_entry_store(url, SHA1(), size, mtime, handler);
  return OldFileEntry(RowId(), url, size, mtime, handler);
}

OldFileEntry
ResourceDatabase::store_old_file_entry(const URL& url, const SHA1& sha1, int size, int mtime, OldFileEntry::Handler handler, const RowId& archive_id)
{
  //RowId file_id = m_file_entry_store(url, sha1, size, mtime, handler);
  return OldFileEntry(RowId(), url, size, mtime, handler);
}

ImageEntry
ResourceDatabase::store_image_entry(const ImageEntry& image)
{
  m_image_entry_store(image);
  return image;
}

bool
ResourceDatabase::get_image_entry(const OldFileEntry& entry, ImageEntry& image_out)
{
  return false; //m_image_entry_get(entry.get_blob_entry().get_id(), image_out);
}

bool
ResourceDatabase::get_old_file_entry(const URL& url, OldFileEntry& entry_out)
{
  return false; //m_file_entry_get_by_url(url, entry_out);
}

void
ResourceDatabase::get_old_file_entries(const std::string& pattern, std::vector<OldFileEntry>& entries_out)
{
  //m_file_entry_get_by_pattern(pattern, entries_out);
}

void
ResourceDatabase::get_old_file_entries(std::vector<OldFileEntry>& entries_out)
{
  //m_file_entry_get_all(entries_out);
}

boost::optional<ResourceEntry>
ResourceDatabase::get_resource_entry(const RowId& blob_id)
{
  //m_resource_entry_get_by_blob_id();
  return boost::optional<ResourceEntry>();
}

void
ResourceDatabase::delete_old_file_entry(const RowId& file_id)
{
  //m_file_entry_delete(file_id);
}

/* EOF */
