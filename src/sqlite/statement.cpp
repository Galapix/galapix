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

#include <sstream>

#include "sqlite/error.hpp"
#include "sqlite/statement.hpp"

SQLiteStatement::SQLiteStatement(SQLiteConnection& db) :
  m_db(db), 
  m_stmt(0),
  m_stmt_str()
{
}

SQLiteStatement::SQLiteStatement(SQLiteConnection& db, const std::string& sqlstmt) :
  m_db(db),
  m_stmt(0),
  m_stmt_str()
{
  prepare(sqlstmt);
}

SQLiteStatement::~SQLiteStatement()
{
retry: 

  switch (sqlite3_finalize(m_stmt))
  {
    case SQLITE_OK:
      break;

    case SQLITE_LOCKED:
      goto retry;

    case SQLITE_BUSY:
      goto retry;

    default:
    {
      std::ostringstream out;
      out << "SQLiteStatement::~SQLiteStatement: " << m_db.get_error_msg();
      throw SQLiteError(out.str());
    }

  }
}

SQLiteStatement&
SQLiteStatement::prepare(const std::string& sqlstmt)
{
  m_stmt_str = sqlstmt;

  if (sqlite3_prepare_v2(m_db.get_db(), sqlstmt.c_str(), -1, &m_stmt,  0) != SQLITE_OK)
  {
    std::ostringstream out;
    out << "SQLiteStatement::prepare: " << m_db.get_error_msg() << ":\n" << sqlstmt;
    throw SQLiteError(out.str());
  }

  return *this;
}

SQLiteStatement&
SQLiteStatement::bind_int(int n, int i)
{
  if (sqlite3_bind_int(m_stmt, n, i) != SQLITE_OK)
  {
    std::ostringstream out;
    out << "SQLiteStatement::bind_int: " << m_db.get_error_msg();
    out << "\n" << m_stmt_str;
    throw SQLiteError(out.str());
  }

  return *this;
}


SQLiteStatement&
SQLiteStatement::bind_int64(int n, int64_t i)
{
  if (sqlite3_bind_int64(m_stmt, n, i) != SQLITE_OK)
  {
    std::ostringstream out;
    out << "SQLiteStatement::bind_int: " << m_db.get_error_msg();
    out << "\n" << m_stmt_str;
    throw SQLiteError(out.str());
  }

  return *this;
}

SQLiteStatement&
SQLiteStatement::bind_text(int n, const std::string& text)
{
  if (sqlite3_bind_text(m_stmt, n, text.c_str(), text.size(), SQLITE_TRANSIENT) != SQLITE_OK)
  {
    std::ostringstream out;
    out << "SQLiteStatement::bind_text: " << m_db.get_error_msg();
    throw SQLiteError(out.str());      
  }

  return *this;
}

SQLiteStatement&
SQLiteStatement::bind_null(int n)
{
  if (sqlite3_bind_null(m_stmt, n) != SQLITE_OK)
  {
    std::ostringstream out;
    out << "SQLiteStatement::bind_text: " << m_db.get_error_msg();
    throw SQLiteError(out.str());      
  }  

  return *this;
}

SQLiteStatement&
SQLiteStatement::bind_blob(int n, const BlobPtr& blob)
{
  if (sqlite3_bind_blob(m_stmt, n, blob->get_data(), blob->size(), SQLITE_TRANSIENT) != SQLITE_OK)
  {
    std::ostringstream out;
    out << "SQLiteStatement::bind_blob: " << m_db.get_error_msg();
    throw SQLiteError(out.str());
  }
  return *this;
}

void
SQLiteStatement::reset()
{
  sqlite3_clear_bindings(m_stmt);  

  if (sqlite3_reset(m_stmt) != SQLITE_OK)
  {
    std::ostringstream out;
    out << "SQLiteStatement::reset: " << m_db.get_error_msg();
    throw SQLiteError(out.str());
  }
}

void 
SQLiteStatement::execute()
{
  if (sqlite3_step(m_stmt) != SQLITE_DONE)
  {
    std::ostringstream out;
    out << "SQLiteStatement::execute: " << m_db.get_error_msg() << ":" << std::endl;
    out << m_stmt_str << std::endl;

    reset();

    throw SQLiteError(out.str());      
  }

  reset();
}

SQLiteReader
SQLiteStatement::execute_query()
{
  return SQLiteReader(m_db, m_stmt);
}

/* EOF */
