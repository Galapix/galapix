/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_RESOURCE_URL_INFO_HPP
#define HEADER_GALAPIX_RESOURCE_URL_INFO_HPP

#include "resource/blob_info.hpp"

class URLInfo
{
public:
  URLInfo() :
    m_id(),
    m_url(),
    m_mtime(-1),
    m_content_type(),
    m_blob_info()
  {}

  URLInfo(const RowId& id, const URLInfo& other) :
    m_id(id),
    m_url(other.m_url),
    m_mtime(other.m_mtime),
    m_content_type(other.m_content_type),
    m_blob_info(other.m_blob_info)
  {}

  URLInfo(const std::string& url,
          long mtime,
          const std::string& content_type,
          const BlobInfo& blob) :
    m_id(),
    m_url(url),
    m_mtime(mtime),
    m_content_type(content_type),
    m_blob_info(blob)
  {}

  std::string get_url() const { return m_url; }
  long get_mtime() const { return m_mtime; }
  std::string get_content_type() const { return m_content_type; }
  BlobInfo get_blob_info() const { return m_blob_info; }

private:
  RowId m_id;
  std::string m_url;
  long m_mtime;
  std::string m_content_type;
  BlobInfo m_blob_info;
};

#endif

/* EOF */
