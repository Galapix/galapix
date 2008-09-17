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

#include "SDL.h"

#include <iostream>
#include <sstream>
#include "sqlite.hpp"

static int busy_callback(void* , int)
{
  SDL_Delay(10);
  return 1;
}

SQLiteError::SQLiteError(const std::string& err_)
  : err(err_)
{  
}

SQLiteConnection::SQLiteConnection(const std::string& filename)
{
  if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK)
    {
      std::ostringstream str; 
      str << "SQLiteConnection: can't open database: " << sqlite3_errmsg(db);
      throw SQLiteError(str.str());
    }

  sqlite3_busy_handler(db, busy_callback, 0);
}

SQLiteConnection::~SQLiteConnection()
{
  sqlite3_close(db);
}

void
SQLiteConnection::exec(const std::string& sqlstmt)
{
  char* errmsg;

  //std::cout << "SQLiteConnection::exec: " << sqlstmt << std::endl;

  if (sqlite3_exec(db, sqlstmt.c_str(), 0, 0, &errmsg) != SQLITE_OK)
    {
      std::ostringstream out;

      out << "FileDatabase: " << errmsg << std::endl;

      sqlite3_free(errmsg);
      errmsg = 0;

      throw SQLiteError(out.str());
    }
}

void
SQLiteConnection::vacuum()
{
  SQLiteStatement stmt(this);
  stmt.prepare("VACUUM;");
  stmt.execute();
}

SQLiteStatement::SQLiteStatement(SQLiteConnection* db)
  : db(db), 
    stmt(0)
{
}

SQLiteStatement::SQLiteStatement(SQLiteConnection* db, const std::string& sqlstmt)
  : db(db),
    stmt(0)
{
  prepare(sqlstmt);
}

SQLiteStatement::~SQLiteStatement()
{
 retry: 

  switch (sqlite3_finalize(stmt))
    {
      case SQLITE_OK:
        break;

      case SQLITE_LOCKED:
        goto retry;

      case SQLITE_BUSY:
        goto retry;

      default:
        {
          std::ostringstream str;
          str << "SQLiteStatement::~SQLiteStatement: " << sqlite3_errmsg(db->get_db());
          throw SQLiteError(str.str());
        }

    }
}

void
SQLiteStatement::prepare(const std::string& sqlstmt)
{
  stmt_str = sqlstmt;

  if (sqlite3_prepare_v2(db->get_db(), sqlstmt.c_str(), -1, &stmt,  0)
      != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteStatement::prepare: " << sqlite3_errmsg(db->get_db()) << ":\n" << sqlstmt;
      throw SQLiteError(str.str());
    }
}

void
SQLiteStatement::bind_int(int n, int i)
{
  if (sqlite3_bind_int(stmt, n, i) != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteStatement::bind_int: " << sqlite3_errmsg(db->get_db());
      str << "\n" << stmt_str;
      throw SQLiteError(str.str());
    }
}

void
SQLiteStatement::bind_text(int n, const std::string& text)
{
  if (sqlite3_bind_text(stmt, n, text.c_str(), text.size(), SQLITE_TRANSIENT) != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteStatement::bind_text: " << sqlite3_errmsg(db->get_db());
      throw SQLiteError(str.str());      
    }
}

void
SQLiteStatement::bind_null(int n)
{
  if (sqlite3_bind_null(stmt, n) != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteStatement::bind_text: " << sqlite3_errmsg(db->get_db());
      throw SQLiteError(str.str());      
    }  
}

void
SQLiteStatement::bind_blob(int n, const Blob& blob)
{
  if (sqlite3_bind_blob(stmt, n, blob.get_data(), blob.size(), SQLITE_TRANSIENT) != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteStatement::bind_blob: " << sqlite3_errmsg(db->get_db());
      throw SQLiteError(str.str());
    }
}

void
SQLiteStatement::reset()
{
  sqlite3_clear_bindings(stmt);  

  if (sqlite3_reset(stmt) != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteStatement::reset: " << sqlite3_errmsg(db->get_db());
      throw SQLiteError(str.str());
    }
}

void 
SQLiteStatement::execute()
{
  if (sqlite3_step(stmt) != SQLITE_DONE)
    {
      std::ostringstream str;
      str << "SQLiteStatement::execute: " << sqlite3_errmsg(db->get_db());
      throw SQLiteError(str.str());      
    }

  reset();
}

SQLiteReader
SQLiteStatement::execute_query()
{
  return SQLiteReader(db, stmt);
}

SQLiteReader::SQLiteReader(SQLiteConnection* db, sqlite3_stmt* stmt)
  : db(db),
    stmt(stmt)
{
}

SQLiteReader::~SQLiteReader()
{  
  // FIXME: Not good, we likely clean up twice
  
  sqlite3_clear_bindings(stmt);  

  if (sqlite3_reset(stmt) != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteReader::~SQLiteReader:" << sqlite3_errmsg(db->get_db());
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
          str << "SQLiteStatement::execute_query: " << sqlite3_errmsg(db->get_db());
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

Blob
SQLiteReader::get_blob(int column)
{
  return Blob(sqlite3_column_blob(stmt, column),
              sqlite3_column_bytes(stmt, column));
}

std::string
SQLiteReader::get_column_name(int column)
{
  return sqlite3_column_name(stmt, column);
}

/* EOF */
