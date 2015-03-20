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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_FILE_ENTRY_GET_BY_PATH_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_FILE_ENTRY_GET_BY_PATH_HPP

#include <boost/optional.hpp>

#include "util/url.hpp"
#include "database/entries/old_file_entry.hpp"

class FileEntryGetByPath
{
private:
  SQLiteStatement m_stmt;

public:
  FileEntryGetByPath(SQLiteConnection& db) :
    m_stmt(db,
           "SELECT\n"
           "  file.id, file.path, file.mtime\n"
           "FROM\n"
           "  file\n"
           "WHERE\n"
           "  file.path = ?1;")
  {}

  boost::optional<FileEntry> operator()(const std::string& path)
  {
    m_stmt.bind_text(1, path);
    SQLiteReader reader = m_stmt.execute_query();

    if (reader.next())
    {
      return FileEntry::from_reader(reader);
    }
    else
    {
      return boost::optional<FileEntry>();
    }
  }

private:
  FileEntryGetByPath(const FileEntryGetByPath&);
  FileEntryGetByPath& operator=(const FileEntryGetByPath&);
};

#endif

/* EOF */
