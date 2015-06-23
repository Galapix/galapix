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
#include "util/raise_exception.hpp"

SQLiteReader::SQLiteReader(SQLiteConnection& db, SQLiteStatement& stmt) :
  m_db(db),
  m_stmt(stmt)
{
}

SQLiteReader::~SQLiteReader()
{
  m_stmt.reset();
}

bool
SQLiteReader::next()
{
retry:
  switch(sqlite3_step(m_stmt.get_stmt()))
  {
    case SQLITE_DONE:
      return false;

    case SQLITE_ROW:
      return true;

    case SQLITE_LOCKED:
    case SQLITE_BUSY:
      goto retry;

    case SQLITE_ERROR:
    case SQLITE_MISUSE:
    default:
    {
      raise_exception(SQLiteError, m_db.get_error_msg());
      return false;
    }
  }
}

int
SQLiteReader::get_int(int column)
{
  return sqlite3_column_int(m_stmt.get_stmt(), column);
}

int64_t
SQLiteReader::get_int64(int column)
{
  return sqlite3_column_int64(m_stmt.get_stmt(), column);
}

float
SQLiteReader::get_float(int column)
{
  return static_cast<float>(get_double(column));
}

double
SQLiteReader::get_double(int column)
{
  return sqlite3_column_double(m_stmt.get_stmt(), column);
}

bool
SQLiteReader::is_null(int column)
{
  return sqlite3_column_type(m_stmt.get_stmt(), column) == SQLITE_NULL;
}

int
SQLiteReader::get_type(int column)
{
  return sqlite3_column_type(m_stmt.get_stmt(), column);
}

std::string
SQLiteReader::get_text(int column)
{
  const void* data = sqlite3_column_text(m_stmt.get_stmt(), column);
  int len = sqlite3_column_bytes(m_stmt.get_stmt(), column);
  return std::string(static_cast<const char*>(data), static_cast<size_t>(len));
}

BlobPtr
SQLiteReader::get_blob(int column)
{
  const void* data = sqlite3_column_blob(m_stmt.get_stmt(), column);
  int len = sqlite3_column_bytes(m_stmt.get_stmt(), column);
  return Blob::copy(data, static_cast<size_t>(len));
}

std::string
SQLiteReader::get_column_name(int column)
{
  return sqlite3_column_name(m_stmt.get_stmt(), column);
}

int
SQLiteReader::get_column_count()
{
  return sqlite3_column_count(m_stmt.get_stmt());
}

/* EOF */
