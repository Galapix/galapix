/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_FILE_INFO_GET_BY_PATH_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_FILE_INFO_GET_BY_PATH_HPP

class FileInfoGetByPath
{
public:
  FileInfoGetByPath(SQLiteConnection& db) :
    m_stmt(db,
           "SELECT\n"
           "  file.id, file.path, file.mtime, blob.sha1, blob.size\n"
           "FROM\n"
           "  file, blob\n"
           "WHERE\n"
           "  file.path = ?1 AND blob.id = file.blob_id;")
  {}

  std::optional<FileInfo> operator()(const std::string& path)
  {
    m_stmt.bind_text(1, path);
    SQLiteReader reader = m_stmt.execute_query();

    if (reader.next())
    {
      return FileInfo(
        RowId(reader.get_int64(0)),
        reader.get_text(1),
        reader.get_int(2),
        SHA1::from_string(reader.get_text(3)),
        static_cast<size_t>(reader.get_int(4))
        );
    }
    else
    {
      return std::optional<FileInfo>();
    }
  }

private:
  SQLiteStatement   m_stmt;

private:
  FileInfoGetByPath(const FileInfoGetByPath&);
  FileInfoGetByPath& operator=(const FileInfoGetByPath&);
};

#endif

/* EOF */
