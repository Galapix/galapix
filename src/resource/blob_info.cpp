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

#include "resource/blob_info.hpp"

#include "resource/blob_accessor.hpp"

BlobInfo
BlobInfo::from_blob(Blob const& blob)
{
  return BlobInfo(SHA1::from_mem(blob), blob.size());
}

BlobInfo
BlobInfo::from_blob(const BlobAccessor& blob_accessor)
{
  if (blob_accessor.has_stdio_name())
  {
    const std::string& path = blob_accessor.get_stdio_name();
    return BlobInfo(SHA1::from_file(path), Filesystem::get_size(path));
  }
  else
  {
    Blob const& blob = blob_accessor.get_blob();
    return BlobInfo(SHA1::from_mem(blob), blob.size());
  }
}

BlobInfo
BlobInfo::from_blob(const BlobAccessorPtr& blob_accessor)
{
  return from_blob(*blob_accessor);
}

/* EOF */
