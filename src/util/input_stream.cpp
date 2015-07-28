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

#include "input_stream.hpp"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>

InputStream
InputStream::from_file(std::string const& filename)
{
  int fd = open(filename.c_str(), O_RDONLY);
  if (fd < 0)
  {
    throw std::system_error(errno, std::system_category());
  }
  else
  {
    return InputStream(fd);
  }
}

InputStream::InputStream(int fd) :
  m_fd(fd)
{
  assert(m_fd > 0);
}

InputStream::InputStream(InputStream&& other) :
  m_fd(other.m_fd)
{
  other.m_fd = 0;
}

InputStream::~InputStream()
{
  if (m_fd)
  {
    close(m_fd);
  }
}

size_t
InputStream::read(void* buffer, size_t len)
{
  ssize_t ret = ::read(m_fd, buffer, len);
  if (ret < 0)
  {
    throw std::system_error(errno, std::system_category());
  }
  else
  {
    return 0;
  }
}

void
InputStream::read_exact(void* buffer, size_t len)
{
  size_t ret = ::read(m_fd, buffer, len);
  if (ret != len)
  {
    std::ostringstream out;
    out << "short read(): " << len << " bytes requested, " << ret << " bytes retrieved";
    throw std::runtime_error(out.str());
  }
}

int16_t swapSLE16(int16_t value)
{
  return value;
}

uint16_t swapULE16(uint16_t value)
{
  return value;
}

int32_t swapSLE32(int32_t value)
{
  return value;
}

uint32_t swapULE32(uint32_t value)
{
  return value;
}

int64_t swapSLE64(int64_t value)
{
  return value;
}

uint64_t swapULE64(uint64_t value)
{
  return value;
}

int16_t swapSBE16(int16_t value)
{
  return __builtin_bswap16(value);
}

uint16_t swapUBE16(uint16_t value)
{
  return __builtin_bswap16(value);
}

int32_t swapSBE32(int32_t value)
{
  return __builtin_bswap32(value);
}

uint32_t swapUBE32(uint32_t value)
{
  return __builtin_bswap32(value);
}

int64_t swapSBE64(int64_t value)
{
  return __builtin_bswap64(value);
}

uint64_t swapUBE64(uint64_t value)
{
  return __builtin_bswap64(value);
}

uint8_t
InputStream::readU8()
{
  uint8_t value;
  read_exact(reinterpret_cast<char*>(&value), sizeof(value));
  return value;
}

int8_t
InputStream::readS8()
{
  int8_t value;
  read_exact(reinterpret_cast<char*>(&value), sizeof(value));
  return value;
}

int16_t
InputStream::readSLE16()
{
  int16_t value;
  read_exact(reinterpret_cast<char*>(&value), sizeof(value));
  return value;
}

int32_t
InputStream::readSLE32()
{
  int32_t value;
  read_exact(reinterpret_cast<char*>(&value), sizeof(value));
  return value;
}

int64_t
InputStream::readSLE64()
{
  int64_t value;
  read_exact(reinterpret_cast<char*>(&value), sizeof(value));
  return value;
}

uint16_t
InputStream::readULE16()
{
  uint16_t value;
  read_exact(reinterpret_cast<char*>(&value), sizeof(value));
  return value;
}

uint32_t
InputStream::readULE32()
{
  uint32_t value;
  read_exact(reinterpret_cast<char*>(&value), sizeof(value));
  return value;
}

uint64_t
InputStream::readULE64()
{
  uint64_t value;
  read_exact(reinterpret_cast<char*>(&value), sizeof(value));
  return value;
}

int16_t
InputStream::readSBE16()
{
  return swapSBE16(readSLE16());
}

int32_t
InputStream::readSBE32()
{
  return swapSBE32(readSLE32());
}

int64_t
InputStream::readSBE64()
{
  return swapSBE64(readSLE64());
}

uint16_t
InputStream::readUBE16()
{
  return swapUBE16(readUBE16());
}

uint32_t
InputStream::readUBE32()
{
  return swapUBE32(readUBE32());
}

uint64_t
InputStream::readUBE64()
{
  return swapUBE64(readUBE64());
}

/* EOF */
