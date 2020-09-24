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

#ifndef HEADER_GALAPIX_UTIL_BLOB_HPP
#define HEADER_GALAPIX_UTIL_BLOB_HPP

#include <span>
#include <memory>
#include <vector>
#include <stdint.h>

/** A Blob a simple wrapper around an read-only array holding raw
    binary data. It is ref counted and provides convinient functions
    to be read and written to a file */
class Blob final
{
public:
  static Blob from_file(std::string const& filename);

  /** Copy the given data into a Blob object */
  static Blob copy(std::vector<uint8_t> data);
  static Blob copy(std::span<uint8_t const> data);

private:
  Blob(std::vector<uint8_t> data);

public:
  Blob();

  size_t size() const;
  uint8_t const* get_data() const;

  std::string str() const;

  void write_to_file(std::string const& filename) const;

  explicit operator bool() const { return m_data != nullptr; }

  std::span<uint8_t const> as_span() { return std::span<uint8_t const>(m_data->begin(), m_data->end()); }

  std::vector<uint8_t>::const_iterator begin() const { return m_data->begin(); }
  std::vector<uint8_t>::const_iterator end() const { return m_data->end(); }

private:
  std::shared_ptr<std::vector<uint8_t> const> m_data;
};

#endif

/* EOF */
