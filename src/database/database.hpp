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

#ifndef HEADER_GALAPIX_DATABASE_DATABASE_HPP
#define HEADER_GALAPIX_DATABASE_DATABASE_HPP

#include <memory>

#include <SQLiteCpp/Database.h>

#include "database/resource_database.hpp"

namespace galapix {

/** Opens resource DB under prefix (`cache4.sqlite3`). Durable tiles live in
 *  thumtoo; Galapix no longer keeps an in-memory tile stub. */
class Database
{
public:
  static Database create(std::string const& prefix);

public:
  Database(std::unique_ptr<SQLite::Database> db,
           std::unique_ptr<ResourceDatabase> resources);
  Database(Database&&) = default;
  ~Database();

  ResourceDatabase& get_resources() { return *m_resources; }

  void delete_file_entry(RowId const& fileid);

  void cleanup();

private:
  std::unique_ptr<SQLite::Database> m_db;
  std::unique_ptr<ResourceDatabase> m_resources;

private:
  Database(Database const&) = delete;
  Database& operator=(Database const&) = delete;
};

} // namespace galapix

#endif

/* EOF */
