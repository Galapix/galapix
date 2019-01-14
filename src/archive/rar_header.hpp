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

#ifndef HEADER_GALAPIX_ARCHIVE_RAR_HEADER_HPP
#define HEADER_GALAPIX_ARCHIVE_RAR_HEADER_HPP

#include <string>
#include <stdint.h>

class RarHeader
{
public:
  static RarHeader from_file(std::string const& rar_filename);

public:
  RarHeader(uint16_t head_flags);

  uint16_t get_flags() const;
  bool is_first_volume() const;
  bool is_multi_volume() const;
  bool has_new_volume_naming() const;
  bool is_encrypted() const;
  bool has_recovery() const;

private:
  uint16_t m_head_flags;

private:
  RarHeader(const RarHeader&) = delete;
  RarHeader& operator=(const RarHeader&) = delete;
};

#endif

/* EOF */
