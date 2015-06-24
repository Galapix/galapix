/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_RESOURCE_ENTRY_GET_BY_BLOB_ID_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_RESOURCE_ENTRY_GET_BY_BLOB_ID_HPP

#include <boost/optional.hpp>

class ResourceEntryGetByBlobId final
{
private:
  SQLiteStatement m_stmt;

public:
  ResourceEntryGetByBlobId(SQLiteConnection& db) :
    m_stmt(db,
           "SELECT\n"
           "  *\n"
           "FROM\n"
           "  resource\n"
           "WHERE\n"
           "  resource.blob_id = ?1;")
  {}

  boost::optional<ResourceEntry> operator()(RowId id)
  {
    m_stmt.bind_int64(1, id.get_id());

    SQLiteReader reader = m_stmt.execute_query();
    if (reader.next())
    {
      //return boost::optional<ResourceEntry>(reader.get_int(0),);
      return boost::optional<ResourceEntry>();
    }
    else
    {
      return boost::optional<ResourceEntry>();
    }
  }

private:
  ResourceEntryGetByBlobId(const ResourceEntryGetByBlobId&);
  ResourceEntryGetByBlobId& operator=(const ResourceEntryGetByBlobId&);
};

#endif

/* EOF */
