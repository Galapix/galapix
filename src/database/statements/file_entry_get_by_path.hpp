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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_FILE_ENTRY_GET_BY_PATH_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_FILE_ENTRY_GET_BY_PATH_HPP

#include <optional>

#include "util/url.hpp"
#include "database/entries/old_file_entry.hpp"

class FileEntryGetByPath
{
public:
  FileEntryGetByPath(SQLite::Database& db) :
    m_stmt(db,
           "SELECT\n"
           "  file.id, file.path, file.mtime\n"
           "FROM\n"
           "  file\n"
           "WHERE\n"
           "  file.path = ?1;")
  {}

  std::optional<FileEntry> operator()(std::string const& path)
  {
    m_stmt.bind(1, path);
    SQLiteReader reader(m_stmt);

    if (reader.next())
    {
      return FileEntry::from_reader(reader);
    }
    else
    {
      return std::optional<FileEntry>();
    }
  }

private:
  SQLite::Statement m_stmt;

private:
  FileEntryGetByPath(FileEntryGetByPath const&);
  FileEntryGetByPath& operator=(FileEntryGetByPath const&);
};

#endif

/* EOF */
