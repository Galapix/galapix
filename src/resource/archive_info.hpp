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

#ifndef HEADER_GALAPIX_RESOURCE_ARCHIVE_INFO_HPP
#define HEADER_GALAPIX_RESOURCE_ARCHIVE_INFO_HPP

#include <boost/optional.hpp>

#include "resource/archive_file_info.hpp"

class ArchiveInfo
{
private:
  std::vector<ArchiveFileInfo> m_files;
  boost::optional<std::string> m_password;

public:
  ArchiveInfo() :
    m_files(),
    m_password()
  {}

  ArchiveInfo(std::vector<ArchiveFileInfo> files,
              const boost::optional<std::string>& password) :
    m_files(std::move(files)),
    m_password(password)
  {}

  std::vector<ArchiveFileInfo> get_files() const { return m_files; }
  boost::optional<std::string> get_password() const { return m_password; }
};

#endif

/* EOF */
