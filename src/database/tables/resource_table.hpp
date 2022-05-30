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

#ifndef HEADER_GALAPIX_DATABASE_TABLES_RESOURCE_TABLE_HPP
#define HEADER_GALAPIX_DATABASE_TABLES_RESOURCE_TABLE_HPP

#include <SQLiteCpp/Database.h>

class ResourceTable
{
public:
  ResourceTable(SQLite::Database& db);

private:
  SQLite::Database& m_db;

private:
  ResourceTable(ResourceTable const&);
  ResourceTable& operator=(ResourceTable const&);
};

#endif

/* EOF */
