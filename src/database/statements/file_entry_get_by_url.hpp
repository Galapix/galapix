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

#ifndef HEADER_GALAPIX_DATABASE_FILE_ENTRY_GET_BY_URL_STATEMENT_HPP
#define HEADER_GALAPIX_DATABASE_FILE_ENTRY_GET_BY_URL_STATEMENT_HPP

class FileEntryGetByUrl
{
private:
  SQLiteStatement m_stmt;

public:
  FileEntryGetByUrl(SQLiteConnection& db) :
    m_stmt(db, 
           "SELECT\n"
           "  file.id, file.url, file.mtime, file.handler, file.parent_file_id, blob.id, blob.sha1, blob.size\n"
           "FROM\n"
           "  file\n"
           "LEFT OUTER JOIN\n"
           "  blob\n"
           "ON\n"
           "  file.blob_id = blob.rowid\n"
           "WHERE\n"
           "  file.url = ?1;")
  {}

  bool operator()(const URL& url, FileEntry& entry_out)
  {
    m_stmt.bind_text(1, url.str());
    SQLiteReader reader = m_stmt.execute_query();

    if (reader.next())
    {
      entry_out = FileEntry::from_reader(reader);
      return true;
    }
    else
    {
      return false;
    }
  }

private:
  FileEntryGetByUrl(const FileEntryGetByUrl&);
  FileEntryGetByUrl& operator=(const FileEntryGetByUrl&);
};

#endif

/* EOF */
