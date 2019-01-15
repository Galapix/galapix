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

#include "sqlite/connection.hpp"

#include <sstream>
#include <unistd.h>

#include "sqlite/error.hpp"

static int busy_callback(void* user_data, int tries)
{
  // FIXME: Is this a good idea?
  usleep(1000*10);
  return 1;
}

SQLiteConnection::SQLiteConnection(const std::string& filename) :
  db(nullptr)
{
  if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK)
  {
    std::ostringstream str;
    str << "SQLiteConnection(): can't open database: " << sqlite3_errmsg(db);
    throw SQLiteError(str.str());
  }

  sqlite3_busy_handler(db, busy_callback, nullptr);

  // FIXME: Kind of the complete wrong place for this
  exec("PRAGMA auto_vacuum = FULL");
  //exec("PRAGMA auto_vacuum = INCREMENTAL");
}

SQLiteConnection::~SQLiteConnection()
{
  sqlite3_close(db);
}

void
SQLiteConnection::exec(const std::string& sqlstmt)
{
  char* errmsg;

  if (sqlite3_exec(db, sqlstmt.c_str(), nullptr, nullptr, &errmsg) != SQLITE_OK)
  {
    std::ostringstream out;

    out << "SQLiteConnection::exec(): " << errmsg << "\n"
        << "  " << sqlstmt << std::endl;

    sqlite3_free(errmsg);
    errmsg = nullptr;

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
