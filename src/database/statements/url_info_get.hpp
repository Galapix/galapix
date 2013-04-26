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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_URL_INFO_GET_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_URL_INFO_GET_HPP

class URLInfoGet
{
private:
  SQLiteStatement m_stmt;

public:
  URLInfoGet(SQLiteConnection& db) :
    m_stmt(db, "")
  {}

  boost::optional<URLInfo> operator()(const std::string& url)
  {
    log_error("not implemented");
    return boost::optional<URLInfo>();
  }

private:
  URLInfoGet(const URLInfoGet&);
  URLInfoGet& operator=(const URLInfoGet&);
};

#endif

/* EOF */
