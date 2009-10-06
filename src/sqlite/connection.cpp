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

#include "sqlite/connection.hpp"

#include <sstream>

#include "error.hpp"

static int busy_callback(void* , int)
{
  // FIXME: Is this a good idea?
  usleep(1000*10);
  return 1;
}

SQLiteConnection::SQLiteConnection(const std::string& filename)
  : db(0)
{
  if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK)
    {
      std::ostringstream str; 
      str << "SQLiteConnection: can't open database: " << sqlite3_errmsg(db);
      throw SQLiteError(str.str());
    }

  sqlite3_busy_handler(db, busy_callback, 0);

  exec("PRAGMA auto_vacuum = 1");
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

      out << "SQLiteConnection: " << errmsg << std::endl;

      sqlite3_free(errmsg);
      errmsg = 0;

      throw SQLiteError(out.str());
    }
  else
    {
      // std::cout << "Executed: " << sqlstmt << std::endl;
    }
}

void
SQLiteConnection::vacuum()
{
  exec("VACUUM;");
}

std::string
SQLiteConnection::get_error_msg()
{
  return sqlite3_errmsg(db);
}

/* EOF */
