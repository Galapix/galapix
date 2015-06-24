/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmail.com>
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

#include "util/sha1.hpp"

#include <algorithm>
#include <mhash.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

#include "util/blob.hpp"
#include "util/raise_exception.hpp"

SHA1
SHA1::from_mem(const uint8_t* data, size_t len)
{
  MHASH td = mhash_init(MHASH_SHA1);
  if (td == MHASH_FAILED)
  {
    raise_runtime_error("Failed to init MHash");
  }
  else
  {
    mhash(td, data, static_cast<mutils_word32>(len));

    SHA1 sha1;
    mhash_deinit(td, sha1.m_data.data());
    return sha1;
  }
}

SHA1
SHA1::from_mem(const std::string& str)
{
  return from_mem(reinterpret_cast<const uint8_t*>(str.data()), str.size());
}

SHA1
SHA1::from_file(const std::string& filename)
{
  MHASH td = mhash_init(MHASH_SHA1);
  if (td == MHASH_FAILED)
  {
    raise_runtime_error("Failed to init MHash");
  }
  else
  {
    const unsigned int buf_size = 32768;
    char buf[buf_size];
    std::ifstream in(filename.c_str(), std::ios::in | std::ios::binary);
    if (!in)
    {
      raise_runtime_error("Couldn't open file " + filename);
    }
    else
    {

      while(!in.eof())
      {
        in.read(buf, buf_size);
        mhash(td, buf, static_cast<mutils_word32>(in.gcount()));
      }

      in.close();

      SHA1 sha1;
      mhash_deinit(td, sha1.m_data.data());
      return sha1;
    }
  }
}

SHA1
SHA1::from_string(const std::string& str)
{
  auto hex2int = [str](char hex)
    {
      if ('0' <= hex && hex <= '9')
      {
        return hex - '0';
      }
      else if ('a' <= hex && hex <= 'f')
      {
        return hex - 'a' + 0xa;
      }
      else if ('A' <= hex && hex <= 'F')
      {
        return hex - 'A' + 0xa;
      }
      else
      {
        raise_runtime_error("not a valid SHA1 string: " + str);
      }
    };

  if (str.size() != 40)
  {
    raise_runtime_error("not a valid SHA1 string: " + str);
  }
  else
  {
    uint8_t data[20];
    for(size_t i = 0; i < 20; ++i)
    {
      data[i] = static_cast<uint8_t>(hex2int(str[2*i]) * 0x10 + hex2int(str[2*i+1]));
    }
    return SHA1(data);
  }
}

SHA1::SHA1() :
  m_data()
{
}

SHA1::SHA1(const BlobPtr& blob) :
  m_data()
{
  std::copy_n(blob->get_data(), blob->size(), m_data.begin());
}

SHA1::SHA1(const uint8_t sha1[20]) :
  m_data()
{
  std::copy_n(sha1, 20, m_data.begin());
}

SHA1::operator bool() const
{
  // a SHA1 that is all zeros is considered invalid
  for(auto v: m_data)
  {
    if (v != 0)
    {
      return true;
    }
  }
  return false;
}

bool
SHA1::operator==(const SHA1& rhs) const
{
  return m_data == rhs.m_data;
}

bool
SHA1::operator!=(const SHA1& rhs) const
{
  return !operator==(rhs);
}


std::string
SHA1::str() const
{
  std::ostringstream out;
  for (mutils_word32 i = 0; i < m_data.size(); ++i)
    out << std::setfill('0') << std::setw(2) << std::hex << int(m_data[i]);
  return out.str();
}

std::ostream& operator<<(std::ostream& os, const SHA1& sha1)
{
  return os << sha1.str();
}

/* EOF */
