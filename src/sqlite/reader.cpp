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

#include "sqlite/reader.hpp"

#include <sstream>

#include <SQLiteCpp/Statement.h>

#include "util/raise_exception.hpp"

namespace galapix {

SQLiteReader::SQLiteReader(SQLite::Statement& stmt) :
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
  return m_stmt.executeStep();
}

int
SQLiteReader::get_int(int column)
{
  return m_stmt.getColumn(column).getInt();
}

int64_t
SQLiteReader::get_int64(int column)
{
  return m_stmt.getColumn(column).getInt64();
}

float
SQLiteReader::get_float(int column)
{
  return static_cast<float>(m_stmt.getColumn(column).getDouble());
}

double
SQLiteReader::get_double(int column)
{
  return m_stmt.getColumn(column).getDouble();
}

bool
SQLiteReader::is_null(int column)
{
  return m_stmt.getColumn(column).isNull();
}

int
SQLiteReader::get_type(int column)
{
   return m_stmt.getColumn(column).getType();
}

std::string
SQLiteReader::get_text(int column)
{
  return m_stmt.getColumn(column).getText();
}

Blob
SQLiteReader::get_blob(int column)
{
  return Blob::copy({
      static_cast<uint8_t const*>(m_stmt.getColumn(column).getBlob()),
      static_cast<size_t>(m_stmt.getColumn(column).size())});
}

std::string
SQLiteReader::get_column_name(int column)
{
  return m_stmt.getColumn(column).getName();
}

int
SQLiteReader::get_column_count()
{
  return m_stmt.getColumnCount();
}

} // namespace galapix

/* EOF */
