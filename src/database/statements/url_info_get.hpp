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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_URL_INFO_GET_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_URL_INFO_GET_HPP

class URLInfoGet
{
public:
  URLInfoGet(SQLite::Database& db) :
    m_stmt(db,
           "SELECT\n"
           "  url.id, url.mtime, url.content_type, blob.sha1, blob.size\n"
           "FROM\n"
           "  url, blob\n"
           "WHERE\n"
           "  url.url = ?1 AND blob.id = url.blob_id;")
  {}

  std::optional<URLInfo> operator()(const std::string& url)
  {
    m_stmt.bind(1, url);
    SQLiteReader reader(m_stmt);

    if (reader.next())
    {
      return URLInfo(
        RowId(reader.get_int64(0)),
        URLInfo(url,
                reader.get_int(1),
                reader.get_text(2),
                BlobInfo(galapix::SHA1::from_string(reader.get_text(3)),
                         static_cast<size_t>(reader.get_int(4))))
        );
    }
    else
    {
      return std::optional<URLInfo>();
    }
  }

private:
  SQLite::Statement m_stmt;

private:
  URLInfoGet(const URLInfoGet&);
  URLInfoGet& operator=(const URLInfoGet&);
};

#endif

/* EOF */
