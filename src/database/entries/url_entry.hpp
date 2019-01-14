/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_DATABASE_ENTRIES_URL_ENTRY_HPP
#define HEADER_GALAPIX_DATABASE_ENTRIES_URL_ENTRY_HPP

#include <string>

#include "database/row_id.hpp"

class SQLiteReader;

class URLEntry
{
public:
  static URLEntry from_reader(SQLiteReader& reader);

public:
  URLEntry(const RowId& id,
           const std::string& host,
           const std::string& path,
           const std::string& content_type,
           long mtime,
           const RowId& blob_id) :
    m_id(id),
    m_host(host),
    m_path(path),
    m_content_type(content_type),
    m_mtime(mtime),
    m_blob_id(blob_id)
  {}

  RowId get_id() const { return m_id; }
  std::string get_host() const { return m_host; }
  std::string get_path() const { return m_path; }
  std::string get_content_type() const { return m_content_type; }
  long get_mtime() const { return m_mtime; }
  RowId get_blob_id() const { return m_blob_id; }

private:
  RowId m_id;
  std::string m_host;
  std::string m_path;
  std::string m_content_type;
  long m_mtime;
  RowId m_blob_id;
};

#endif

/* EOF */
