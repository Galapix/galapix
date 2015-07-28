/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2015 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_UTIL_INPUT_STREAM_HPP
#define HEADER_GALAPIX_UTIL_INPUT_STREAM_HPP

#include <string>

class InputStream
{
public:
  static InputStream from_file(std::string const& filename);

public:
  InputStream(InputStream&& other);
  InputStream(int fd);
  ~InputStream();

  size_t read(void* buffer, size_t len);
  void read_exact(void* buffer, size_t len);

  uint8_t readU8();
  int8_t readS8();

  int16_t readSLE16();
  int32_t readSLE32();
  int64_t readSLE64();

  uint16_t readULE16();
  uint32_t readULE32();
  uint64_t readULE64();

  int16_t readSBE16();
  int32_t readSBE32();
  int64_t readSBE64();

  uint16_t readUBE16();
  uint32_t readUBE32();
  uint64_t readUBE64();

private:
  int m_fd;

private:
  InputStream(const InputStream&) = delete;
  InputStream& operator=(const InputStream&) = delete;
};

#endif

/* EOF */
