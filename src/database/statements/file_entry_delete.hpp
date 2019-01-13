/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_FILE_ENTRY_DELETE_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_FILE_ENTRY_DELETE_HPP

#include "database/row_id.hpp"
#include "sqlite/statement.hpp"

class FileEntryDelete
{
public:
  FileEntryDelete(SQLiteConnection& db) :
    m_db(db),
    m_stmt(db, "DELETE FROM file WHERE id = ?1;")
  {}

  void operator()(const RowId& fileid)
  {
    assert(fileid);
    m_stmt.bind_int64(1, fileid.get_id());
    m_stmt.execute();
  }

private:
  SQLiteConnection& m_db;
  SQLiteStatement   m_stmt;

private:
  FileEntryDelete(const FileEntryDelete&);
  FileEntryDelete& operator=(const FileEntryDelete&);
};

#endif

/* EOF */
