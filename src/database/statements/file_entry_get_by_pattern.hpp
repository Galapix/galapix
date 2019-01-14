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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_FILE_ENTRY_GET_BY_PATTERN_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_FILE_ENTRY_GET_BY_PATTERN_HPP

#include "database/entries/file_entry.hpp"

class FileEntryGetByPattern
{
public:
  FileEntryGetByPattern(SQLiteConnection& db) :
    m_stmt(db,
           "SELECT\n"
           "  file.id, file.path, file.mtime, file.blob_id\n"
           "FROM\n"
           "  file\n"
           "WHERE\n"
           "  file.path GLOB ?1;")
  {}

  void operator()(const std::string& pattern, std::vector<FileEntry>& entries_out)
  {
    m_stmt.bind_text(1, pattern);
    SQLiteReader reader = m_stmt.execute_query();

    while (reader.next())
    {
      entries_out.push_back(FileEntry::from_reader(reader));
    }
  }

private:
  SQLiteStatement m_stmt;

private:
  FileEntryGetByPattern(const FileEntryGetByPattern&);
  FileEntryGetByPattern& operator=(const FileEntryGetByPattern&);
};

#endif

/* EOF */
