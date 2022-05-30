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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_BLOB_INFO_STORE_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_BLOB_INFO_STORE_HPP

#include <SQLiteCpp/Database.h>
#include <SQLiteCpp/Statement.h>

namespace galapix {

class BlobInfoStore
{
public:
  BlobInfoStore(SQLite::Database& db) :
    m_db(db),
    m_stmt(db, "INSERT INTO blob (sha1, size) VALUES (?1, ?2);")
  {}

  RowId operator()(BlobInfo const& blob_info)
  {
    m_stmt.bind(1, blob_info.get_sha1().str());
    m_stmt.bind(2, static_cast<int64_t>(blob_info.get_size()));
    m_stmt.exec();

    return RowId(m_db.getLastInsertRowid());
  }

private:
  SQLite::Database& m_db;
  SQLite::Statement m_stmt;

private:
  BlobInfoStore(BlobInfoStore const&);
  BlobInfoStore& operator=(BlobInfoStore const&);
};

} // namespace galapix

#endif

/* EOF */
