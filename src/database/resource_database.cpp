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

#include "database/resource_database.hpp"

#include <iostream>

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

#include <logmich/log.hpp>
#include <surf/software_surface.hpp>
#include <surf/software_surface_factory.hpp>

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

using namespace galapix;

ResourceDatabase::ResourceDatabase(SQLite::Database& db) :
  m_db(db),

  m_archive_table(m_db),
  m_blob_table(m_db),
  m_file_table(m_db),
  m_image_table(m_db),
  m_resource_table(m_db),
  m_url_table(m_db),
  m_video_table(m_db),
  m_file_info_store(std::make_unique<FileInfoStore>(m_db)),
  m_file_info_get_by_path(std::make_unique<FileInfoGetByPath>(m_db)),
  m_image_info_store(std::make_unique<ImageInfoStore>(m_db)),
  m_image_info_get(std::make_unique<ImageInfoGet>(m_db)),
  m_url_info_store(std::make_unique<URLInfoStore>(m_db)),
  m_url_info_get(std::make_unique<URLInfoGet>(m_db)),
  m_resource_info_get(std::make_unique<ResourceInfoGet>(m_db)),
  m_resource_info_store(std::make_unique<ResourceInfoStore>(m_db)),
  m_blob_info_store(std::make_unique<BlobInfoStore>(m_db)),
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

std::optional<ImageInfo>
ResourceDatabase::get_image_info(ResourceInfo const& resource)
{
  return (*m_image_info_get)(resource);
}

RowId
ResourceDatabase::store_image_info(ImageInfo const& image_info)
{
  return (*m_image_info_store)(image_info);
}

std::optional<ResourceInfo>
ResourceDatabase::get_resource_info(ResourceLocator const& locator, BlobInfo const& blob)
{
  return (*m_resource_info_get)(locator, blob);
}

std::optional<ResourceInfo>
ResourceDatabase::get_resource_info(BlobInfo const& blob)
{
  return (*m_resource_info_get)(blob);
}

RowId
ResourceDatabase::store_resource_info(ResourceInfo const& info)
{
  return (*m_resource_info_store)(info);
}

std::optional<URLInfo>
ResourceDatabase::get_url_info(std::string const& url)
{
  return (*m_url_info_get)(url);
}

RowId
ResourceDatabase::store_url_info(URLInfo const& url_info)
{
  if (!url_info.get_blob_info().get_id())
  {
    (*m_blob_info_store)(url_info.get_blob_info());
  }
  return (*m_url_info_store)(url_info);
}

std::optional<FileInfo>
ResourceDatabase::get_file_info(std::string const& path)
{
  return (*m_file_info_get_by_path)(path);
}

RowId
ResourceDatabase::store_file_info(FileInfo const& file_info)
{
  if (!file_info.get_blob_info().get_id())
  {
    (*m_blob_info_store)(file_info.get_blob_info());
  }
  return (*m_file_info_store)(file_info);
}

std::optional<FileEntry>
ResourceDatabase::get_file_entry(std::string const& path)
{
  return m_file_entry_get_by_path(path);
}

void
ResourceDatabase::get_file_entries(std::vector<FileEntry>& entries_out)
{
  return m_file_entry_get_all(entries_out);
}

void
ResourceDatabase::get_file_entries(std::string const& pattern, std::vector<FileEntry>& entries_out)
{
  return m_file_entry_get_by_pattern(pattern, entries_out);
}

void
ResourceDatabase::store_file_entry(std::string const& path, int mtime)
{
  m_file_entry_store(path, mtime);
}

OldFileEntry
ResourceDatabase::store_old_file_entry(URL const& url, int size, int mtime, OldFileEntry::Handler handler)
{
  //RowId file_id = m_file_entry_store(url, SHA1(), size, mtime, handler);
  return OldFileEntry(RowId(), url, size, mtime, handler);
}

OldFileEntry
ResourceDatabase::store_old_file_entry(URL const& url, SHA1 const& sha1, int size, int mtime, OldFileEntry::Handler handler, RowId const& archive_id)
{
  //RowId file_id = m_file_entry_store(url, sha1, size, mtime, handler);
  return OldFileEntry(RowId(), url, size, mtime, handler);
}

ImageEntry
ResourceDatabase::store_image_entry(ImageEntry const& image)
{
  m_image_entry_store(image);
  return image;
}

bool
ResourceDatabase::get_image_entry(OldFileEntry const& entry, ImageEntry& image_out)
{
  return false; //m_image_entry_get(entry.get_blob_entry().get_id(), image_out);
}

bool
ResourceDatabase::get_old_file_entry(URL const& url, OldFileEntry& entry_out)
{
  return false; //m_file_entry_get_by_url(url, entry_out);
}

void
ResourceDatabase::get_old_file_entries(std::string const& pattern, std::vector<OldFileEntry>& entries_out)
{
  //m_file_entry_get_by_pattern(pattern, entries_out);
}

void
ResourceDatabase::get_old_file_entries(std::vector<OldFileEntry>& entries_out)
{
  //m_file_entry_get_all(entries_out);
}

std::optional<ResourceEntry>
ResourceDatabase::get_resource_entry(RowId const& blob_id)
{
  //m_resource_entry_get_by_blob_id();
  return std::optional<ResourceEntry>();
}

void
ResourceDatabase::delete_old_file_entry(RowId const& file_id)
{
  //m_file_entry_delete(file_id);
}

/* EOF */
