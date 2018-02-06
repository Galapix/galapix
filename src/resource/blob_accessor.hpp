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

#ifndef HEADER_GALAPIX_RESOURCE_BLOB_ACCESSOR_HPP
#define HEADER_GALAPIX_RESOURCE_BLOB_ACCESSOR_HPP

#include <string>
#include <mutex>
#include <optional>

#include "util/blob.hpp"
#include "resource/blob_info.hpp"

class BlobAccessor
{
private:
  mutable std::mutex m_mutex;
  mutable std::string m_filename;
  mutable Blob m_blob;
  mutable std::optional<BlobInfo> m_blob_info;

public:
  BlobAccessor(const std::string& filename);
  BlobAccessor(Blob blob);

  bool has_stdio_name() const;
  std::string get_stdio_name() const;

  bool has_blob() const;
  Blob get_blob() const;

  size_t size() const;
  const uint8_t* get_data() const;

  BlobInfo get_blob_info() const;

private:
  BlobAccessor(const BlobAccessor&);
  BlobAccessor& operator=(const BlobAccessor&);
};

typedef std::shared_ptr<BlobAccessor> BlobAccessorPtr;

#endif

/* EOF */
