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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_FILE_INFO_STORE_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_FILE_INFO_STORE_HPP

class FileInfoStore
{
private:
  SQLiteConnection& m_db;
  SQLiteStatement   m_blob_stmt;
  SQLiteStatement   m_file_stmt;

public:
  FileInfoStore(SQLiteConnection& db) :
    m_db(db),
    m_blob_stmt(db, "INSERT INTO blob (sha1, size) VALUES (?1, ?2);"),
    m_file_stmt(db, "INSERT OR REPLACE INTO file (path, mtime, blob_id) VALUES (?1, ?2, last_insert_rowid());")
  {}

  RowId operator()(const FileInfo& file_info)
  {
    m_blob_stmt.bind_text(1, file_info.get_sha1().str());
    m_blob_stmt.bind_int(2, file_info.get_size());
    m_blob_stmt.execute();

    m_file_stmt.bind_text (1, file_info.get_path());
    m_file_stmt.bind_int  (2, file_info.get_mtime());
    m_file_stmt.execute();
  
    return RowId{sqlite3_last_insert_rowid(m_db.get_db())};
  }

private:
  FileInfoStore(const FileInfoStore&) = delete;
  FileInfoStore& operator=(const FileInfoStore&) = delete;
};

#endif

/* EOF */
