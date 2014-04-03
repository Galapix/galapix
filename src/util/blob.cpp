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

#include "util/blob.hpp"

#include <assert.h>
#include <stdexcept>
#include <fstream>
#include <string.h>

#include "util/raise_exception.hpp"

Blob::Blob(std::vector<uint8_t> data) :
  m_data(std::move(data))
{
}

Blob::Blob(const void* data, int len) :
  m_data(len)
{
  memcpy(m_data.data(), data, m_data.size());
}

Blob::Blob(int len) :
  m_data(len)
{
}

int
Blob::size() const
{
  return m_data.size();
}

const uint8_t*
Blob::get_data() const
{
  return m_data.data();
}

std::string
Blob::str() const
{
  return std::string(reinterpret_cast<const char*>(m_data.data()), m_data.size());
}

void
Blob::write_to_file(const std::string& filename)
{
  std::ofstream out(filename.c_str(), std::ios::binary);
  out.write(reinterpret_cast<const char*>(m_data.data()), m_data.size());
}


BlobPtr
Blob::create(int len)
{
  return BlobPtr(new Blob(len));
}

BlobPtr
Blob::from_file(const std::string& filename)
{
  std::ifstream in(filename.c_str(), std::ios::binary);
  if (!in)
  {
    raise_runtime_error("Blob::from_file(): Couldn't read " + filename);
  }
  else
  {
    std::vector<uint8_t> data;
    uint8_t buffer[4096];
    while(!in.eof())
    {
      int len = in.read(reinterpret_cast<char*>(buffer), 4096).gcount();
      std::copy(buffer, buffer+len, std::back_inserter(data));
    }

    // FIXME: useless copy, should read directly into the blob
    return Blob::copy(data);
  }
}

BlobPtr
Blob::copy(const void* data, int len)
{
  return BlobPtr(new Blob(data, len));
}

BlobPtr
Blob::copy(std::vector<uint8_t> data)
{
  return BlobPtr(new Blob(std::move(data)));
}

/* EOF */
