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

#ifndef HEADER_GALAPIX_SQLITE_STATEMENT_HPP
#define HEADER_GALAPIX_SQLITE_STATEMENT_HPP

#include "sqlite/reader.hpp"

class SQLiteStatement
{
public:
  SQLiteStatement(SQLiteConnection& db);
  SQLiteStatement(SQLiteConnection& db, const std::string& sqlstmt);
  ~SQLiteStatement();

  SQLiteStatement& prepare(const std::string& sqlstmt);

  void clear_bindings();
  void reset();

  SQLiteStatement& bind_null(int n);
  SQLiteStatement& bind_int(int n, int i);
  SQLiteStatement& bind_int64(int n, int64_t i);
  SQLiteStatement& bind_text(int n, const std::string&);
  SQLiteStatement& bind_blob(int n, Blob const&);
  SQLiteStatement& bind_blob(int n, const uint8_t* data, size_t len);

  void execute();
  SQLiteReader execute_query();

  std::string str() const { return m_stmt_str; }

  sqlite3_stmt*  get_stmt() { return m_stmt; }

private:
  SQLiteConnection& m_db;
  sqlite3_stmt*  m_stmt;
  std::string    m_stmt_str;

private:
  SQLiteStatement(const SQLiteStatement&);
  SQLiteStatement& operator=(const SQLiteStatement&);
};

#endif

/* EOF */
