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

#ifndef HEADER_GALAPIX_DATABASE_STATEMENTS_IMAGE_ENTRY_GET_HPP
#define HEADER_GALAPIX_DATABASE_STATEMENTS_IMAGE_ENTRY_GET_HPP

#include "database/entries/image_entry.hpp"

#include <logmich/log.hpp>

class ImageEntryGet final
{
public:
  ImageEntryGet(SQLiteConnection& db) :
    m_db(db),
    m_stmt(db, "SELECT id, resource_id, width, height FROM image WHERE id = ?1;")
  {}

  bool operator()(const RowId& image_id, ImageEntry& image_out)
  {
    log_debug("looking up: %d", image_id);
    m_stmt.bind_int64(1, image_id.get_id());

    SQLiteReader reader = m_stmt.execute_query();
    if (reader.next())
    {
      image_out = ImageEntry(reader.get_int64(0),
                             reader.get_int64(1),
                             reader.get_int(2),
                             reader.get_int(3));
      return true;
    }
    else
    {
      return false;
    }
  }

private:
  SQLiteConnection& m_db;
  SQLiteStatement   m_stmt;

private:
  ImageEntryGet(const ImageEntryGet&);
  ImageEntryGet& operator=(const ImageEntryGet&);
};

#endif

/* EOF */
