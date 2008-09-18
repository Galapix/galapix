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

#include <stdexcept>
#include <fstream>
#include <string.h>
#include "blob.hpp"

class BlobImpl
{
public:
  uint8_t* data;
  int      len;

  BlobImpl(const void* data_, int len_)
  {
    data = new uint8_t[len_];
    len  = len_;

    memcpy(data, data_, len);
  }

  BlobImpl(const std::vector<uint8_t>& data_in)
  {
    data = new uint8_t[data_in.size()];
    len  = data_in.size();

    memcpy(data, &*data_in.begin(), len);
  }

  ~BlobImpl()
  {
    delete[] data;
  }
};

Blob::Blob(const std::vector<uint8_t>& data)
  : impl(new BlobImpl(data))
{}

Blob::Blob(const void* data, int len)
  : impl(new BlobImpl(data, len))
{}

Blob::Blob()
{}

int
Blob::size() const 
{
  if (impl.get())
    return impl->len; 
  else
    return 0;
}

uint8_t* 
Blob::get_data() const 
{
  if (impl.get())
    return impl->data; 
  else
    return 0;
}

std::string
Blob::str() const
{
  if (impl.get())
    return std::string(reinterpret_cast<char*>(impl->data), impl->len);
  else
    return std::string();
}

void
Blob::write_to_file(const std::string& filename)
{
  std::ofstream out(filename.c_str(), std::ios::binary);
  out.write(reinterpret_cast<char*>(impl->data), impl->len);
}

Blob
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
      // FIXME: Useless copy again
      return Blob(data);
    }
}

/* EOF */
