/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_DATABASE_FILE_ENTRY_STORE_STATEMENT_HPP
#define HEADER_GALAPIX_DATABASE_FILE_ENTRY_STORE_STATEMENT_HPP

#include <iostream>
#include <assert.h>

#include "util/sha1.hpp"

class FileEntryStore
{
private:
  SQLiteConnection& m_db;
  SQLiteStatement   m_blob_select_stmt;
  SQLiteStatement   m_blob_stmt;
  SQLiteStatement   m_file_stmt;

public:
  FileEntryStore(SQLiteConnection& db) :
    m_db(db),
    m_blob_select_stmt(db, "SELECT id FROM blob WHERE sha1 = ?1;\n"),
    m_blob_stmt(db, "INSERT OR REPLACE INTO blob (sha1, size) VALUES (?1, ?2);\n"),
    m_file_stmt(db, "INSERT OR REPLACE INTO file (url, mtime, handler, blob_id) VALUES (?1, ?2, ?3, ?4);")
  {}

  RowId operator()(const URL& url, const SHA1& sha1, int size, int mtime, int handler)
  {
    int64_t blob_id;
  
    if (!sha1)
    {
      m_blob_stmt.bind_null(1);
      m_blob_stmt.bind_int64(2, size);
      m_blob_stmt.execute();
      blob_id = sqlite3_last_insert_rowid(m_db.get_db());
    }
    else
    {
      m_blob_select_stmt.bind_blob(1, sha1.data(), sha1.size());
      SQLiteReader reader = m_blob_select_stmt.execute_query();
      if (reader.next())
      {
        blob_id = reader.get_int64(0);
      }
      else
      {
        m_blob_stmt.bind_blob(1, sha1.data(), sha1.size());
        m_blob_stmt.bind_int64(2, size);
        m_blob_stmt.execute();
        blob_id = sqlite3_last_insert_rowid(m_db.get_db());
      }
    }

    m_file_stmt.bind_text (1, url.str());
    m_file_stmt.bind_int  (2, mtime);
    m_file_stmt.bind_int  (3, handler);
    m_file_stmt.bind_int64(4, blob_id);
    m_file_stmt.execute();
  
    return RowId{sqlite3_last_insert_rowid(m_db.get_db())};
  }

private:
  FileEntryStore(const FileEntryStore&);
  FileEntryStore& operator=(const FileEntryStore&);
};

#endif

/* EOF */
