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

#ifndef HEADER_GALAPIX_SQLITE_READER_HPP
#define HEADER_GALAPIX_SQLITE_READER_HPP

#include <SQLiteCpp/Column.h>
#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

#include "util/blob.hpp"

namespace galapix {

class SQLiteReader
{
public:
  SQLiteReader(SQLite::Statement& stmt);
  ~SQLiteReader();

  bool next();

  bool        is_null(int column);
  int         get_type(int column);
  float       get_float(int column);
  double      get_double(int column);
  int         get_int(int column);
  int64_t     get_int64(int column);
  std::string get_text(int column);
  Blob     get_blob(int column);

  std::string get_column_name(int column);
  int get_column_count();

private:
  SQLiteReader& operator=(SQLiteReader const&);

private:
  SQLite::Statement& m_stmt;
};

} // namespace galapix

#endif

/* EOF */
