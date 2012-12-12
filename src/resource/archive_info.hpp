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

#ifndef HEADER_GALAPIX_GALAPIX_ARCHIVE_INFO_HPP
#define HEADER_GALAPIX_GALAPIX_ARCHIVE_INFO_HPP

#include "resource/file_info.hpp"

class ArchiveInfo
{
private:
  std::vector<FileInfo> m_files;
  std::string m_password;
  bool m_has_password;

public:
  ArchiveInfo();

  std::vector<FileInfo> get_files() const { return m_files; }
  std::string get_password() const { return m_password; }
  bool has_password() const { return m_has_password; }

private:
  ArchiveInfo(const ArchiveInfo&);
  ArchiveInfo& operator=(const ArchiveInfo&);
};

#endif

/* EOF */
