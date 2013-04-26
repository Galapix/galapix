/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmx.de>
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

class URLInfo
{
private:
  RowId m_id;
  long m_mtime;
  std::string m_content_type;
  SHA1 m_sha1;

public:
  URLInfo() :
    m_id(),
    m_mtime(-1),
    m_content_type(),
    m_sha1()
  {}

  URLInfo(const RowId& id, const URLInfo& other) :
    m_id(id),
    m_mtime(other.m_mtime),
    m_content_type(other.m_content_type),
    m_sha1(other.m_sha1)
  {}

  URLInfo(long mtime,
          const std::string& content_type,
          const SHA1& sha1) :
    m_id(),
    m_mtime(mtime),
    m_content_type(content_type),
    m_sha1(sha1)
  {}

  long get_mtime() const { return m_mtime; }
  std::string get_content_type() const { return m_content_type; }
  SHA1 get_sha1() const { return m_sha1; }
};

#endif

/* EOF */
