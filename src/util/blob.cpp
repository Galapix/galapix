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

#include <stdexcept>
#include <fstream>
#include <string.h>

Blob::Blob(const std::vector<uint8_t>& data) :
  m_data(new uint8_t[data.size()]),
  m_len(data.size())
{
  memcpy(m_data.get(), &*data.begin(), m_len);
}

Blob::Blob(const void* data, int len) :
  m_data(new uint8_t[len]),
  m_len(len)
{
  memcpy(m_data.get(), data, m_len);
}

Blob::Blob(int len) :
  m_data(new uint8_t[len]),
  m_len(len)
{  
}

int
Blob::size() const 
{
  return m_len;
}

uint8_t* 
Blob::get_data() const 
{
  return m_data.get();
}

std::string
Blob::str() const
{
  return std::string(reinterpret_cast<char*>(m_data.get()), m_len);
}

void
Blob::write_to_file(const std::string& filename)
{
  std::ofstream out(filename.c_str(), std::ios::binary);
  out.write(reinterpret_cast<char*>(m_data.get()), m_len);
}

void
Blob::append(const void* data, int len)
{
  assert(!"Blob::append(const void* data, int len): Implement me");
}

BlobHandle
Blob::create(int len)
{
  return BlobHandle(new Blob(len));  
}

BlobHandle
Blob::from_file(const std::string& filename)
{
  std::ifstream in(filename.c_str(), std::ios::binary);
  if (!in)
    {
      throw std::runtime_error("Blob::from_file: Couldn't read " + filename);
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

BlobHandle
Blob::copy(const void* data, int len)
{
  return BlobHandle(new Blob(data, len));
}

BlobHandle
Blob::copy(const std::vector<uint8_t>& data)
{
  return BlobHandle(new Blob(data));
}

/* EOF */
