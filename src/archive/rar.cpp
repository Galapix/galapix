/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#include "archive/rar.hpp"

#include <stdexcept>
#include <iostream>
#include <fstream>

#include "util/exec.hpp"
#include "util/raise_exception.hpp"

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

RarHeader
Rar::read_header(std::string const& rar_filename)
{
  std::ifstream in;
  in.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  in.open(rar_filename, std::ios::binary);

  // See: http://forensicswiki.org/wiki/RAR

  // MARK_HEAD
  uint8_t mark_head[7];
  in.read(reinterpret_cast<char*>(mark_head), sizeof(mark_head));

  // FIXME: check magic

  // MAIN_HEAD
  uint16_t head_crc;
  uint8_t head_type;
  uint16_t head_flags;
  uint16_t head_size;

  in.read(reinterpret_cast<char*>(&head_crc), sizeof(head_crc));
  in.read(reinterpret_cast<char*>(&head_type), sizeof(head_type));
  in.read(reinterpret_cast<char*>(&head_flags), sizeof(head_flags));
  in.read(reinterpret_cast<char*>(&head_size), sizeof(head_size));

  //std::cout << head_crc << " " << head_type << " " << head_flags << " " << head_size << std::endl;

  // multi-part first: 0x111
  // multi-part rest:   0x11
  return RarHeader(head_flags);
}

std::vector<std::string>
Rar::get_filenames(const std::string& archive)
{
  Exec rar("rar");
  rar.arg("vb").arg("-p-").arg(archive);
  if (rar.exec() == 0)
  {
    std::vector<std::string> lst;
    const std::vector<char>& stdout_lst = rar.get_stdout();
    std::vector<char>::const_iterator start = stdout_lst.begin();
    for(std::vector<char>::const_iterator i = stdout_lst.begin(); i != stdout_lst.end(); ++i)
    {
      if (*i == '\n')
      {
        lst.push_back(std::string(start, i));
        start = i+1;
      }
    }
    return lst;
  }
  else
  {
    raise_runtime_error(std::string(rar.get_stderr().begin(), rar.get_stderr().end()));
    return std::vector<std::string>();
  }
}

BlobPtr
Rar::get_file(const std::string& archive, const std::string& filename)
{
  Exec rar("rar");
  rar.arg("p").arg("-inul").arg("-p-").arg(archive).arg(filename);
  if (rar.exec() == 0)
  {
    // FIXME: Unneeded copy of data
    return Blob::copy(&*rar.get_stdout().begin(), rar.get_stdout().size());
  }
  else
  {
    raise_runtime_error(rar.str() + "\n" + std::string(rar.get_stderr().begin(), rar.get_stderr().end()));
    return BlobPtr();
  }
}

void
Rar::extract(const std::string& archive, const std::string& target_directory)
{
  Exec rar("rar");
  rar.arg("x").arg("-inul").arg("-w" + target_directory).arg(archive);
  if (rar.exec() != 0)
  {
    raise_runtime_error(rar.str() + "\n" + std::string(rar.get_stderr().begin(), rar.get_stderr().end()));
  }
}

/* EOF */
