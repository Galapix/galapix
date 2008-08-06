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

  if (sqlite3_exec(db, sqlstmt.c_str(), 0, 0, &errmsg) != SQLITE_OK)
    {
      std::ostringstream out;

      out << "FileDatabase: " << errmsg << std::endl;

      sqlite3_free(errmsg);
      errmsg = 0;

      throw SQLiteError(out.str());
    }
}

SQLiteStatement::SQLiteStatement(SQLiteConnection* db, const std::string& sqlstmt)
{
  if (sqlite3_prepare_v2(db->get_db(),
                         "INSERT OR REPLACE INTO files (filename, md5, filesize, width, height, mtime)"
                         "           VALUES (      ?1,  ?2,       ?3,    ?4,     ?5,    ?6);", -1, &stmt,  0)
      != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteStatement: " << sqlite3_errmsg(db->get_db());
      throw SQLiteError(str.str());
    }
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
  if (sqlite3_bind_text(stmt, n, text.c_str(), text.size(), SQLITE_STATIC) != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteStatement: " << sqlite3_errmsg(db->get_db());
      throw SQLiteError(str.str());      
    }
}

void
SQLiteStatement::bind_blob(int n, const std::string& blob)
{
  if (sqlite3_bind_blob(stmt, n, blob.c_str(), blob.size(), SQLITE_STATIC) != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteStatement: " << sqlite3_errmsg(db->get_db());
      throw SQLiteError(str.str());
    }
}

void
SQLiteStatement::reset()
{
  if (sqlite3_reset(stmt) != SQLITE_OK)
    {
      std::ostringstream str;
      str << "SQLiteStatement: " << sqlite3_errmsg(db->get_db());
      throw SQLiteError(str.str());
    }
  
  sqlite3_clear_bindings(stmt);  
}

void
SQLiteStatement::step()
{
  if (sqlite3_step(stmt) != SQLITE_DONE)
    {
      std::ostringstream str;
      str << "SQLiteStatement: " << sqlite3_errmsg(db->get_db());
      throw SQLiteError(str.str());     
    }
}

/* EOF */
