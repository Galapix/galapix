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

#ifndef HEADER_GALAPIX_DATABASE_FILE_ENTRY_HPP
#define HEADER_GALAPIX_DATABASE_FILE_ENTRY_HPP

#include <memory>
#include <assert.h>

#include "database/file_id.hpp"
#include "math/math.hpp"
#include "math/size.hpp"
#include "util/url.hpp"

class SQLiteReader;

class FileEntry 
{
public:
  FileEntry() :
    m_fileid(),
    m_url(),
    m_size(),
    m_mtime(),
    m_format()
  {}

  FileEntry(const FileId& fileid,
            const URL& url,
            int size,
            int mtime,
            int format) :
    m_fileid(fileid),
    m_url(url),
    m_size(size),
    m_mtime(mtime),
    m_format(format)
  {}

  FileEntry(SQLiteReader& reader);

  FileId get_fileid()     const { return m_fileid; }
  URL    get_url()        const { return m_url; }
  int    get_size()  const { return m_size;  }
  int    get_mtime() const { return m_mtime; }

private:
  /** Unique id by which one can refer to this FileEntry, used in the
      'tile' table in the database */
  FileId m_fileid;

  /** The URL of the image file */
  URL m_url;

  /** Size of the file in bytes */
  int m_size;

  /** Last modification of the file */
  int m_mtime;

  /** Format of the file */
  int m_format;
};

std::ostream& operator<<(std::ostream& os, const FileEntry& entry);

#endif

/* EOF */
