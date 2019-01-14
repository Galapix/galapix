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

#include "util/blob.hpp"

#include <assert.h>
#include <stdexcept>
#include <fstream>
#include <string.h>

#include "util/raise_exception.hpp"

Blob::Blob() :
  m_data()
{
}

Blob::Blob(std::vector<uint8_t> data) :
  m_data(std::make_shared<std::vector<uint8_t> >(std::move(data)))
{
}

Blob::Blob(void const* data, size_t len) :
  m_data(std::make_shared<std::vector<uint8_t> >(
           static_cast<uint8_t const*>(data),
           static_cast<uint8_t const*>(data) + len))
{
}

size_t
Blob::size() const
{
  if (!m_data)
  {
    return 0;
  }
  else
  {
    return m_data->size();
  }
}

const uint8_t*
Blob::get_data() const
{
  if (!m_data)
  {
    return nullptr;
  }
  else
  {
    return m_data->data();
  }
}

std::string
Blob::str() const
{
  if (!m_data)
  {
    return {};
  }
  else
  {
    return std::string(reinterpret_cast<const char*>(m_data->data()), m_data->size());
  }
}

void
Blob::write_to_file(const std::string& filename) const
{
  std::ofstream out(filename.c_str(), std::ios::binary);
  if (!out)
  {
    raise_runtime_error("Blob::write_to_file(): Couldn't write to " + filename);
  }
  else
  {
    if (m_data)
    {
      out.write(reinterpret_cast<const char*>(m_data->data()),
                static_cast<std::streamsize>(m_data->size()));
    }
  }
}

Blob
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
      std::streamsize len = in.read(reinterpret_cast<char*>(buffer), 4096).gcount();
      std::copy(buffer, buffer + len, std::back_inserter(data));
    }

    return Blob::copy(std::move(data));
  }
}

Blob
Blob::copy(void const* data, size_t len)
{
  return Blob(data, len);
}

Blob
Blob::copy(std::vector<uint8_t> data)
{
  return Blob(std::move(data));
}

/* EOF */
