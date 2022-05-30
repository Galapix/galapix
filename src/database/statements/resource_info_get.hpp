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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_RESOURCE_INFO_GET_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_RESOURCE_INFO_GET_HPP

#include "resource/resource_info.hpp"
#include "resource/resource_locator.hpp"

class ResourceInfoGet
{
public:
  ResourceInfoGet(SQLite::Database& db) :
    m_stmt(db,
           "SELECT\n"
           "  resource.id, resource.type, resource.handler, resource.arguments, resource.status\n"
           "FROM\n"
           "  resource, blob\n"
           "WHERE\n"
           "  resource.blob_id = blob.id AND blob.sha1 = ?;"),
    m_blob_stmt(db,
           "SELECT\n"
           "  resource.id, resource.type, resource.handler, resource.arguments, resource.status\n"
           "FROM\n"
           "  resource\n"
           "WHERE\n"
           "  resource.blob_id = ?1;")
  {}

  std::optional<ResourceInfo> operator()(BlobInfo const& blob)
  {
    m_blob_stmt.bind(1, blob.get_id().get_id());

    SQLiteReader reader(m_blob_stmt);
    if (reader.next())
    {
      return std::make_optional<ResourceInfo>(
        RowId(reader.get_int64(0)),
        ResourceName(BlobInfo(),
                     ResourceHandler(reader.get_text(1),
                                     reader.get_text(2),
                                     reader.get_text(3))),
        static_cast<ResourceStatus>(reader.get_int(4)));
    }
    else
    {
      return {};
    }
  }

  /** scan through all resource entries trying to find one that matches the given handler */
  std::optional<ResourceInfo> operator()(ResourceLocator const& locator, BlobInfo const& blob)
  {
    m_stmt.bind(1, blob.get_sha1().str());

    SQLiteReader reader(m_stmt);

    while(reader.next())
    {
      std::string type    = reader.get_text(1);
      std::string handler = reader.get_text(2);
      std::string args    = reader.get_text(3);

      if (locator.get_handler().empty())
      {
        // locator doesn't provide us with a handler, so just return the first we find
        return ResourceInfo(RowId(reader.get_int64(0)),
                            ResourceName(blob,
                                         ResourceHandler(reader.get_text(2), reader.get_text(3), reader.get_text(4))),
                            ResourceStatus_from_string(reader.get_text(5)));
      }
      else if (type    == locator.get_handler().back().get_type() ||
               handler == locator.get_handler().back().get_name() ||
               args    == locator.get_handler().back().get_args())
      {
        return ResourceInfo(RowId(reader.get_int64(0)),
                            ResourceName(blob, locator.get_handler().back()),
                            ResourceStatus_from_string(reader.get_text(5)));
      }
    }

    return std::optional<ResourceInfo>();
  }

private:
  SQLite::Statement m_stmt;
  SQLite::Statement m_blob_stmt;

private:
  ResourceInfoGet(ResourceInfoGet const&);
  ResourceInfoGet& operator=(ResourceInfoGet const&);
};

#endif

/* EOF */
