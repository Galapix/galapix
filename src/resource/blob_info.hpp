/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_RESOURCE_BLOB_INFO_HPP
#define HEADER_GALAPIX_RESOURCE_BLOB_INFO_HPP

#include <memory>

#include "database/row_id.hpp"
#include "util/blob.hpp"
#include "util/sha1.hpp"
#include "util/filesystem.hpp"

class BlobAccessor;
typedef std::shared_ptr<BlobAccessor> BlobAccessorPtr;

class BlobInfo
{
private:
  RowId m_id;
  SHA1 m_sha1;
  size_t  m_size;

public:
  static BlobInfo from_blob(BlobPtr blob);
  static BlobInfo from_blob(const BlobAccessor& blob_accessor);
  static BlobInfo from_blob(const BlobAccessorPtr& blob_accessor);

  BlobInfo() :
    m_id(),
    m_sha1(),
    m_size()
  {}

  BlobInfo(const RowId& id,
           const SHA1& sha1,
           size_t size) :
    m_id(id),
    m_sha1(sha1),
    m_size(size)
  {}

  BlobInfo(const SHA1& sha1,
           size_t size) :
    m_id(),
    m_sha1(sha1),
    m_size(size)
  {}

  RowId get_id() const { return m_id; }
  SHA1 get_sha1() const { return m_sha1; }
  size_t get_size() const { return m_size; }
};

#endif

/* EOF */
