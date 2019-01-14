/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
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

#include "archive/rar_header.hpp"

#include <fstream>
#include <string.h>
#include <stdexcept>
#include <exception>
#include <sstream>

#include "util/input_stream.hpp"

RarHeader
RarHeader::from_file(std::string const& rar_filename)
{
  // See: http://forensicswiki.org/wiki/RAR

  try
  {
    InputStream in = InputStream::from_file(rar_filename);

    uint8_t rar_magic[7] = { 0x52, 0x61, 0x72, 0x21, 0x1A, 0x07, 0x00 };
    uint8_t mark_head[7];
    in.read(mark_head, sizeof(mark_head));

    if (memcmp(rar_magic, mark_head, sizeof(rar_magic)) != 0)
    {
      throw std::runtime_error("rar magic doesn't match");
    }
    else
    {
      // MAIN_HEAD
      uint16_t head_crc __attribute__ ((unused)) = in.readULE16();
      uint8_t head_type = in.readU8();
      uint16_t head_flags = in.readULE16();
      uint16_t head_size __attribute__ ((unused)) = in.readULE16();

      if (head_type != 0x73)
      {
        throw std::runtime_error("header type mismatch");
      }
      else
      {
        return RarHeader(head_flags);
      }
    }
  }
  catch(...)
  {
    std::ostringstream out;
    out << rar_filename << ": RarHeader::from_file() failed";
    std::throw_with_nested(std::runtime_error(out.str()));
  }
}

RarHeader::RarHeader(uint16_t head_flags) :
  m_head_flags(head_flags)
{
}

uint16_t
RarHeader::get_flags() const
{
  return m_head_flags;
}

bool
RarHeader::is_first_volume() const
{
  return m_head_flags & 0x100;
}

bool
RarHeader::is_multi_volume() const
{
  return m_head_flags & 0x01;
}

bool
RarHeader::has_new_volume_naming() const
{
  return m_head_flags & 0x10;
}

bool
RarHeader::is_encrypted() const
{
  return m_head_flags & 0x80;
}

bool
RarHeader::has_recovery() const
{
  return m_head_flags & 0x40;
}

/* EOF */
