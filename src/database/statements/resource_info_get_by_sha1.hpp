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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_RESOURCE_INFO_GET_BY_SHA1_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_RESOURCE_INFO_GET_BY_SHA1_HPP

class ResourceInfoGetBySHA1
{
private:
  SQLiteStatement   m_stmt;

public:
  ResourceInfoGetBySHA1(SQLiteConnection& db) :
    m_stmt(db,
           "SELECT\n"
           "  resource.type, resource.handler, resource.arguments\n"
           "FROM\n"
           "  resource, blob\n"
           "WHERE\n"
           "  resource.blob_id = blob.id AND blob.sha1 = ?;")
  {}

  std::optional<ResourceInfo> operator()(const SHA1& sha1)
  {
    m_stmt.bind_text(1, path);
    SQLiteReader reader = m_stmt.execute_query();

    if (reader.next())
    {
      return ResourceInfo(reader.get_text(0),
                          reader.get_text(1),
                          reader.get_text(2));
    }
    else
    {
      return std::optional<ResourceInfo>();
    }
  }

private:
  ResourceInfoGetBySHA1(const ResourceInfoGetBySHA1&);
  ResourceInfoGetBySHA1& operator=(const ResourceInfoGetBySHA1&);
};

#endif

/* EOF */
