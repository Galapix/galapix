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

#ifndef HEADER_GALAPIX_DATABASE_ENTRIES_BLOB_ENTRY_HPP
#define HEADER_GALAPIX_DATABASE_ENTRIES_BLOB_ENTRY_HPP

#include "database/row_id.hpp"
#include "util/sha1.hpp"

class BlobEntry
{
public:
  BlobEntry() :
    m_id(),
    m_sha1(),
    m_size()
  {}

  BlobEntry(galapix::SHA1 const& sha1, int size) :
    m_id(),
    m_sha1(sha1),
    m_size(size)
  {}

  BlobEntry(RowId const& id, galapix::SHA1 const& sha1, int size) :
    m_id(id),
    m_sha1(sha1),
    m_size(size)
  {}

  RowId get_id() const   { return m_id;   }
  int   get_size() const { return m_size; }
  galapix::SHA1 const& get_sha1() const { return m_sha1; }

private:
  RowId m_id;
  galapix::SHA1  m_sha1;
  int   m_size;
};

#endif

/* EOF */
