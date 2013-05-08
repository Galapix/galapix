/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_RESOURCE_FILE_INFO_HPP
#define HEADER_GALAPIX_RESOURCE_FILE_INFO_HPP

#include <string>

#include "util/sha1.hpp"
#include "database/row_id.hpp"
#include "resource/blob_info.hpp"

class FileInfo
{
private:
  RowId m_id;
  std::string m_path;
  int  m_mtime;
  BlobInfo m_blob_info;

public:
  static FileInfo from_file(const std::string& filename);

  FileInfo(const RowId& id, const FileInfo& rhs) :
    m_id(id),
    m_path(rhs.m_path),
    m_mtime(rhs.m_mtime),
    m_blob_info(rhs.m_blob_info)
  {}

  FileInfo(const RowId& id, const std::string& path, int mtime, const SHA1& sha1, int size) :
    m_id(id),
    m_path(path),
    m_mtime(mtime),
    m_blob_info(sha1, size)
  {}
 
  FileInfo(const std::string& path, int mtime, const SHA1& sha1, int size) :
    m_id(),
    m_path(path),
    m_mtime(mtime),
    m_blob_info(sha1, size)
  {}
    
  RowId get_id() const { return m_id; }
  std::string get_path() const { return m_path; }
  int  get_mtime() const { return m_mtime; }
  BlobInfo get_blob_info() const { return m_blob_info; }

  SHA1 get_sha1() const { return m_blob_info.get_sha1(); }
  int  get_size() const { return m_blob_info.get_size(); }
};

#endif

/* EOF */
