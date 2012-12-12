/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_RESOURCE_FILE_INFO_HPP
#define HEADER_GALAPIX_RESOURCE_FILE_INFO_HPP

#include <string>

#include "util/sha1.hpp"

class FileInfo
{
private:
  SHA1 m_sha1;
  int  m_mtime;

public:
  static FileInfo from_file(const std::string& filename);
  
  SHA1 get_sha1() const { return m_sha1; }
  int  get_mtime() const { return m_mtime; }
  
private:
  FileInfo(SHA1 sha1, int mtime) :
    m_sha1(sha1),
    m_mtime(mtime)
  {}
};

#endif

/* EOF */
