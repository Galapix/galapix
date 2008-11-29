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

#ifndef HEADER_SQLITE_CONNECTION_HPP
#define HEADER_SQLITE_CONNECTION_HPP

#include <sqlite3.h>
#include <string>

class SQLiteConnection
{
private:
  sqlite3* db;

public:
  SQLiteConnection(const std::string& filename);
  ~SQLiteConnection();

  void exec(const std::string& sqlstmt);

  /** Do a VACCUM on the database to clean up collected garbage, this
      call can take quite a while (~1min) for larger databases, since
      the whole database gets copied in the process */
  void vacuum();
  
  std::string get_error_msg();

  sqlite3* get_db() const { return db; }
};

#endif

/* EOF */
