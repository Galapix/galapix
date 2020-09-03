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

#include <sstream>
#include <assert.h>
#include <logmich/log.hpp>

#include "sqlite/error.hpp"
#include "util/raise_exception.hpp"

#include "sqlite/statement.hpp"

SQLiteStatement::SQLiteStatement(SQLiteConnection& db) :
  m_db(db),
  m_stmt(nullptr),
  m_stmt_str()
{
}

SQLiteStatement::SQLiteStatement(SQLiteConnection& db, const std::string& sqlstmt) :
  m_db(db),
  m_stmt(nullptr),
  m_stmt_str()
{
  prepare(sqlstmt);
}

SQLiteStatement::~SQLiteStatement()
{
  int ret;
  do {
    ret = sqlite3_finalize(m_stmt);
  } while(ret == SQLITE_LOCKED ||
          ret == SQLITE_BUSY);

  if (ret != SQLITE_OK)
  {
    log_error(m_db.get_error_msg());
  }
}

SQLiteStatement&
SQLiteStatement::prepare(const std::string& sqlstmt)
{
  assert(m_stmt == nullptr);

  m_stmt_str = sqlstmt;

  if (sqlite3_prepare_v2(m_db.get_db(), sqlstmt.c_str(), -1, &m_stmt,  nullptr) != SQLITE_OK)
  {
    raise_exception(SQLiteError, "in\n" << m_stmt_str << "\n" << m_db.get_error_msg());
  }

  return *this;
}

void
SQLiteStatement::clear_bindings()
{
  sqlite3_clear_bindings(m_stmt);
}



SQLiteStatement&
SQLiteStatement::bind_int(int n, int i)
{
  if (sqlite3_bind_int(m_stmt, n, i) != SQLITE_OK)
  {
    raise_exception(SQLiteError, "in\n" << m_stmt_str << "\n" << m_db.get_error_msg());
  }

  return *this;
}


SQLiteStatement&
SQLiteStatement::bind_int64(int n, int64_t i)
{
  if (sqlite3_bind_int64(m_stmt, n, i) != SQLITE_OK)
  {
    raise_exception(SQLiteError, "in\n" << m_stmt_str << "\n" << m_db.get_error_msg());
  }

  return *this;
}

SQLiteStatement&
SQLiteStatement::bind_text(int n, const std::string& text)
{
  if (sqlite3_bind_text(m_stmt, n, text.c_str(), static_cast<int>(text.size()), SQLITE_TRANSIENT) != SQLITE_OK)
  {
    raise_exception(SQLiteError, "in\n" << m_stmt_str << "\n" << m_db.get_error_msg());
  }

  return *this;
}

SQLiteStatement&
SQLiteStatement::bind_null(int n)
{
  if (sqlite3_bind_null(m_stmt, n) != SQLITE_OK)
  {
    raise_exception(SQLiteError, "in\n" << m_stmt_str << "\n" << m_db.get_error_msg());
  }

  return *this;
}

SQLiteStatement&
SQLiteStatement::bind_blob(int n, Blob const& blob)
{
  if (sqlite3_bind_blob(m_stmt, n, blob.get_data(), static_cast<int>(blob.size()), SQLITE_TRANSIENT) != SQLITE_OK)
  {
    raise_exception(SQLiteError, "in\n" << m_stmt_str << "\n" << m_db.get_error_msg());
  }
  return *this;
}

SQLiteStatement&
SQLiteStatement::bind_blob(int n, std::span<uint8_t const> data)
{
  if (sqlite3_bind_blob(m_stmt, n, data.data(), static_cast<int>(data.size()), SQLITE_TRANSIENT) != SQLITE_OK)
  {
    raise_exception(SQLiteError, "in\n" << m_stmt_str << "\n" << m_db.get_error_msg());
  }
  return *this;
}

void
SQLiteStatement::reset()
{
  if (sqlite3_reset(m_stmt) != SQLITE_OK)
  {
    raise_exception(SQLiteError, "in\n" << m_stmt_str << "\n" << m_db.get_error_msg());
  }
}

void
SQLiteStatement::execute()
{
  if (sqlite3_step(m_stmt) != SQLITE_DONE)
  {
    std::ostringstream out;
    out << "SQLiteStatement::execute(): " << m_db.get_error_msg() << ":" << std::endl;
    out << m_stmt_str << std::endl;

    reset();
    clear_bindings();

    throw SQLiteError(out.str());
  }
  else
  {
    reset();
    clear_bindings();
  }
}

SQLiteReader
SQLiteStatement::execute_query()
{
  return SQLiteReader(m_db, *this);
}

/* EOF */
