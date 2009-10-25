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

#include "sqlite/reader.hpp"

#include <sstream>

#include "sqlite/error.hpp"
#include "sqlite/statement.hpp"

SQLiteReader::SQLiteReader(SQLiteConnection& db, sqlite3_stmt* stmt) :
  m_db(db),
  m_stmt(stmt)
{
}

SQLiteReader::~SQLiteReader()
{
  sqlite3_clear_bindings(m_stmt);
  if (sqlite3_reset(m_stmt) != SQLITE_OK)
  {
    std::ostringstream str;
    str << "SQLiteReader::~SQLiteReader:" << m_db.get_error_msg();
    throw SQLiteError(str.str());
  }
}

bool
SQLiteReader::next()
{
retry:
  switch(sqlite3_step(m_stmt))
  {
    case SQLITE_DONE:
      return false;

    case SQLITE_ROW:
      return true;

    case SQLITE_BUSY:
      goto retry;        
        
    case SQLITE_ERROR:
    case SQLITE_MISUSE:
    default:
    {
      std::ostringstream str;
      str << "SQLiteReader::next: " << m_db.get_error_msg();
      throw SQLiteError(str.str());     
      return false;
    }
  }
}

int
SQLiteReader::get_int(int column)
{
  return sqlite3_column_int(m_stmt, column);
}

int64_t
SQLiteReader::get_int64(int column)
{
  return sqlite3_column_int64(m_stmt, column);
}

bool
SQLiteReader::is_null(int column)
{
  return sqlite3_column_type(m_stmt, column) == SQLITE_NULL;
}

int
SQLiteReader::get_type(int column)
{
  return sqlite3_column_type(m_stmt, column);
}

std::string
SQLiteReader::get_text(int column)
{
  const void* data = sqlite3_column_text(m_stmt, column);
  int len = sqlite3_column_bytes(m_stmt, column);
  return std::string(static_cast<const char*>(data), len);
}

BlobPtr
SQLiteReader::get_blob(int column)
{
  return Blob::copy(sqlite3_column_blob(m_stmt, column),
                    sqlite3_column_bytes(m_stmt, column));
}

std::string
SQLiteReader::get_column_name(int column)
{
  return sqlite3_column_name(m_stmt, column);
}

/* EOF */
