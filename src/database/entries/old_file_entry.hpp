/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_DATABASE_ENTRIES_OLD_FILE_ENTRY_HPP
#define HEADER_GALAPIX_DATABASE_ENTRIES_OLD_FILE_ENTRY_HPP

#include <memory>
#include <assert.h>

#include "database/entries/blob_entry.hpp"
#include "database/row_id.hpp"
#include "math/math.hpp"
#include "math/size.hpp"
#include "util/url.hpp"

class SQLiteReader;

class OldFileEntry final
{
public:
  enum Handler {
    kUnknownHandler,
    kUnhandledHandler,
    kImageHandler,
    kArchiveHandler
  };

public:
  static OldFileEntry from_reader(SQLiteReader& reader);

  OldFileEntry() :
    m_id(),
    m_url(),
    m_mtime(),
    m_handler(),
    m_blob_entry(),
    m_parent()
  {}

  OldFileEntry(const RowId& id,
            const URL& url,
            int size,
            int mtime,
            Handler handler) :
    m_id(id),
    m_url(url),
    m_mtime(mtime),
    m_handler(handler),
    m_blob_entry(),
    m_parent()
  {}

  OldFileEntry(const RowId& id,
            const URL& url,
            int mtime,
            Handler handler,
            const RowId& parent,
            const BlobEntry& blob_entry) :
    m_id(id),
    m_url(url),
    m_mtime(mtime),
    m_handler(handler),
    m_blob_entry(blob_entry),
    m_parent(parent)
  {}

  RowId   get_id()  const { return m_id; }
  URL     get_url()     const { return m_url;    }
  int     get_mtime()   const { return m_mtime;  }
  Handler get_handler() const { return m_handler; }
  const BlobEntry& get_blob_entry() const { return m_blob_entry; }
  RowId   get_parent() const { return m_parent; }

private:
  /** Unique id by which one can refer to this OldFileEntry, used in the
      'tile' table in the database */
  RowId m_id;

  /** The URL of the image file */
  URL m_url;

  /** Last modification of the file */
  int m_mtime;

  /** Handler of the file */
  Handler m_handler;

  BlobEntry m_blob_entry;

  RowId m_parent;
};

std::ostream& operator<<(std::ostream& os, const OldFileEntry& entry);

#endif

/* EOF */
