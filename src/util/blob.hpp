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

#ifndef HEADER_GALAPIX_UTIL_BLOB_HPP
#define HEADER_GALAPIX_UTIL_BLOB_HPP

#include <memory>
#include <vector>
#include <stdint.h>

class Blob;

typedef std::shared_ptr<Blob> BlobPtr;

/** A Blob a simple wrapper around an array holding raw binary data.
    It is ref counted and provides convinient functions to be read and
    written to a file */
class Blob
{
private:
  std::vector<uint8_t> m_data;

private:
  Blob(std::vector<uint8_t> data);
  Blob(const void* data, size_t len);
  Blob(size_t len);

public:
  size_t size() const;
  const uint8_t* get_data() const;

  std::string str() const;

  void write_to_file(const std::string& filename);

public:
  static BlobPtr create(int len);

  static BlobPtr from_file(const std::string& filename);

  /** Copy the given data into a Blob object */
  static BlobPtr copy(const void* data, size_t len);
  static BlobPtr copy(std::vector<uint8_t> data);
};

#endif

/* EOF */
