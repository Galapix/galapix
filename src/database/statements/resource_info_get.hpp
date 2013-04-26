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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_RESOURCE_INFO_GET_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_RESOURCE_INFO_GET_HPP

#include "resource/resource_info.hpp"

class ResourceInfoGet
{
private:
  SQLiteStatement m_stmt;

public:
  ResourceInfoGet(SQLiteConnection& db) :
    m_stmt(db,
           "SELECT\n"
           "  resource.id, resource.type, resource.handler, resource.arguments, resource.status\n"
           "FROM\n"
           "  resource, blob\n"
           "WHERE\n"
           "  resource.blob_id = blob.id AND blob.sha1 = ?;")
  {}

  /** scan through all resource entries trying to find one that matches the given handler */
  boost::optional<ResourceInfo> operator()(const ResourceLocator& locator, const SHA1& sha1)
  {
    m_stmt.bind_text(1, sha1.str());

    SQLiteReader reader = m_stmt.execute_query();
    
    while(reader.next())
    {
      std::string type    = reader.get_text(1);
      std::string handler = reader.get_text(2);
      std::string args    = reader.get_text(3);

      if (locator.get_handler().empty())
      {
        // locator doesn't provide us with a handler, so just return the first we find
        return ResourceInfo(RowId(reader.get_int64(0)), sha1, locator.get_handler().back(),
                            ResourceStatus_from_string(reader.get_text(5)));
      }
      else if (type    == locator.get_handler().back().get_type() ||
               handler == locator.get_handler().back().get_name() ||
               args    == locator.get_handler().back().get_args())
      {
        return ResourceInfo(RowId(reader.get_int64(0)), sha1, locator.get_handler().back(),
                            ResourceStatus_from_string(reader.get_text(5)));
      }
    }

    return boost::optional<ResourceInfo>();
  }

private:
  ResourceInfoGet(const ResourceInfoGet&);
  ResourceInfoGet& operator=(const ResourceInfoGet&);
};

#endif

/* EOF */
