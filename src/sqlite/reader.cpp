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

SQLiteReader::SQLiteReader(SQLiteConnection* db_, sqlite3_stmt* stmt_) :
  db(db_),
  stmt(stmt_)
{
}

SQLiteReader::SQLiteReader(const SQLiteReader& rhs) :
  db(rhs.db),
  stmt(rhs.stmt)
{ // FIXME: WRONG, leads to double free
}

SQLiteReader&
SQLiteReader::operator=(const SQLiteReader& rhs)
{ // FIXME: WRONG, leads to double free
  db   = rhs.db;
  stmt = rhs.stmt;  
  return *this;
}

SQLiteReader::~SQLiteReader()
{  
  // FIXME: Not good, we likely clean up twice
  sqlite3_clear_bindings(stmt);  

  if (sqlite3_reset(stmt) != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteReader::~SQLiteReader:" << db->get_error_msg();
      throw SQLiteError(str.str());
    }
}

bool
SQLiteReader::next()
{
 retry:

  switch(sqlite3_step(stmt))
    {
      case SQLITE_DONE:
        // cleanup here or in the destructor?!
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
          str << "SQLiteStatement::execute_query: " << db->get_error_msg();
          throw SQLiteError(str.str());     
          return false;
        }
    }
}

int
SQLiteReader::get_int(int column)
{
  return sqlite3_column_int(stmt, column);
}

int64_t
SQLiteReader::get_int64(int column)
{
  return sqlite3_column_int64(stmt, column);
}

bool
SQLiteReader::is_null(int column)
{
  return sqlite3_column_type(stmt, column) == SQLITE_NULL;
}

int
SQLiteReader::get_type(int column)
{
  return sqlite3_column_type(stmt, column);
}

std::string
SQLiteReader::get_text(int column)
{
  const void* data = sqlite3_column_text(stmt, column);
  int len = sqlite3_column_bytes(stmt, column);
  return std::string(static_cast<const char*>(data), len);
}

BlobHandle
SQLiteReader::get_blob(int column)
{
  return Blob::copy(sqlite3_column_blob(stmt, column),
                    sqlite3_column_bytes(stmt, column));
}

std::string
SQLiteReader::get_column_name(int column)
{
  return sqlite3_column_name(stmt, column);
}

/* EOF */
