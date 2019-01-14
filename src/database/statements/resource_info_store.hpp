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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_RESOURCE_INFO_STORE_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_RESOURCE_INFO_STORE_HPP

class ResourceInfoStore
{
public:
  ResourceInfoStore(SQLiteConnection& db) :
    m_db(db),
    m_stmt(db,
           "INSERT INTO resource (blob_id, type, handler, arguments, status) VALUES (?1, ?2, ?3, ?4, ?5);")
  {}

  RowId operator()(const ResourceInfo& info)
  {
    m_stmt.bind_int64(1, info.get_name().get_blob_info().get_id().get_id());
    m_stmt.bind_text(2, info.get_handler().get_type());
    m_stmt.bind_text(3, info.get_handler().get_name());
    m_stmt.bind_text(4, info.get_handler().get_args());
    m_stmt.bind_int(5, static_cast<int>(info.get_status()));
    m_stmt.execute();

    return {sqlite3_last_insert_rowid(m_db.get_db())};
  }

private:
  SQLiteConnection& m_db;
  SQLiteStatement m_stmt;

private:
  ResourceInfoStore(const ResourceInfoStore&);
  ResourceInfoStore& operator=(const ResourceInfoStore&);
};

#endif

/* EOF */
