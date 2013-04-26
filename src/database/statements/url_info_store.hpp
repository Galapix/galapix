/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_URL_INFO_STORE_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_URL_INFO_STORE_HPP

class URLInfoStore
{
private:
  SQLiteStatement m_stmt;

public:
  URLInfoStore(SQLiteConnection& db) :
    m_stmt(db, "")
  {}

  RowId operator()(const URLInfo& url_info)
  {
    log_error("not implemented");
    return RowId();
  }

private:
  URLInfoStore(const URLInfoStore&);
  URLInfoStore& operator=(const URLInfoStore&);
};

#endif

/* EOF */
