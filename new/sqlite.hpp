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

#ifndef HEADER_SQLITE_HPP
#define HEADER_SQLITE_HPP

#include <stdexcept>
#include <sqlite3.h>

class SQLiteError : public std::exception
{
private:
  std::string err;

public:
  SQLiteError(const std::string& err);
  virtual ~SQLiteError() throw () {}

  const char* what() const throw () { return err.c_str(); }
};

class SQLiteConnection
{
private:
  sqlite3* db;

public:
  SQLiteConnection(const std::string& filename);
  ~SQLiteConnection();

  void exec(const std::string& sqlstmt);

  sqlite3* get_db() const { return db; }
};

class SQLiteReader
{
private:
  SQLiteConnection* db;
  sqlite3_stmt*   stmt;
  
public:
  SQLiteReader(SQLiteConnection* db, sqlite3_stmt* stmt);
  ~SQLiteReader();

  bool next();

  int         get_int(int column);
  std::string get_text(int column);
  std::string get_blob(int column);

  std::string get_column_name(int column);
};

class SQLiteStatement
{
private:
  SQLiteConnection* db;
  sqlite3_stmt*   stmt;

  void reset();
  
public:
  SQLiteStatement(SQLiteConnection* db);
  SQLiteStatement(SQLiteConnection* db, const std::string& sqlstmt);
  ~SQLiteStatement();

  void prepare(const std::string& sqlstmt);

  void bind_int(int n, int i);
  void bind_text(int n, const std::string&);
  void bind_blob(int n, const std::string&);

  void execute();
  SQLiteReader execute_query();
};

#endif

/* EOF */
