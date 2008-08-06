/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#include <iostream>
#include <sstream>
#include "sqlite.hpp"

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

SQLiteStatement::SQLiteStatement(SQLiteConnection* db)
  : db(db), 
    stmt(0)
{
}

SQLiteStatement::SQLiteStatement(SQLiteConnection* db, const std::string& sqlstmt)
  : db(db)
{
  prepare(sqlstmt);
}

SQLiteStatement::~SQLiteStatement()
{
  if (sqlite3_finalize(stmt) != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteStatement: " << sqlite3_errmsg(db->get_db());
      throw SQLiteError(str.str());
    }
}

void
SQLiteStatement::prepare(const std::string& sqlstmt)
{
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
      str << "SQLiteStatement: " << sqlite3_errmsg(db->get_db());
      throw SQLiteError(str.str());
    }
}

void
SQLiteStatement::bind_text(int n, const std::string& text)
{
  if (sqlite3_bind_text(stmt, n, text.c_str(), text.size(), SQLITE_TRANSIENT) != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteStatement: " << sqlite3_errmsg(db->get_db());
      throw SQLiteError(str.str());      
    }
}

void
SQLiteStatement::bind_blob(int n, const std::string& blob)
{
  if (sqlite3_bind_blob(stmt, n, blob.c_str(), blob.size(), SQLITE_TRANSIENT) != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteStatement: " << sqlite3_errmsg(db->get_db());
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
      str << "SQLiteStatement: " << sqlite3_errmsg(db->get_db());
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
      str << "SQLiteStatement: " << sqlite3_errmsg(db->get_db());
      throw SQLiteError(str.str());
    }
}

bool
SQLiteReader::next()
{
  switch(sqlite3_step(stmt))
    {
      case SQLITE_DONE:
        // cleanup here or in the destructor?!
        return false;

      case SQLITE_ROW:
        return true;

      case SQLITE_BUSY:
        throw SQLiteError("BUSY");
        return false;

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

std::string
SQLiteReader::get_text(int column)
{
  const void* data = sqlite3_column_text(stmt, column);
  int len = sqlite3_column_bytes(stmt, column);
  return std::string(static_cast<const char*>(data), len);
}

std::string
SQLiteReader::get_blob(int column)
{
  const void* data = sqlite3_column_blob(stmt, column);
  int len = sqlite3_column_bytes(stmt, column);
  return std::string(static_cast<const char*>(data), len);
}

std::string
SQLiteReader::get_column_name(int column)
{
  return sqlite3_column_name(stmt, column);
}

/* EOF */
