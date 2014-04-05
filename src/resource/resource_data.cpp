/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmx.de>
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

#include "resource/resource_data.hpp"

SourceType
ResourceData::get_source_type() const
{
  return m_source_type;
}

URLInfo
ResourceData::get_url_info() const
{
  assert(m_source_type == SourceType::URL);
  return m_url_info;
}

FileInfo
ResourceData::get_file_info() const
{
  assert(m_source_type == SourceType::File);
  return m_file_info;
}

ImageInfo
ResourceData::get_image_info() const
{
  assert(get_type() == ResourceType::Image);

  return m_image_info;
}

ArchiveInfo
ResourceData::get_archive_info() const
{
  assert(get_type() == ResourceType::Archive);

  return m_archive_info;
}

std::vector<ResourceName>
ResourceData::get_children() const
{
  return m_children;
}


ResourceType
ResourceData::get_type() const
{
  return ResourceType_from_string(m_info.get_name().get_handler().get_type());
}

/* EOF */