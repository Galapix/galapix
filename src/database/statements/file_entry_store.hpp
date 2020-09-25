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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_FILE_ENTRY_STORE_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_FILE_ENTRY_STORE_HPP

#include <iostream>
#include <assert.h>

class FileEntryStore
{
public:
  FileEntryStore(SQLite::Database& db) :
    m_db(db),
    m_file_stmt(db, "INSERT OR REPLACE INTO file (path, mtime) VALUES (?1, ?2);")
  {}

  RowId operator()(const std::string& path, long mtime)
  {
    m_file_stmt.bind(1, path);
    m_file_stmt.bind(2, mtime);
    m_file_stmt.exec();

    return RowId(m_db.getLastInsertRowid());
  }

private:
  SQLite::Database& m_db;
  SQLite::Statement m_file_stmt;

private:
  FileEntryStore(const FileEntryStore&);
  FileEntryStore& operator=(const FileEntryStore&);
};

#endif

/* EOF */
