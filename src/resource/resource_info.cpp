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

#include "resource/resource_info.hpp"

ResourceInfo::ResourceInfo() :
  m_id(),
  m_name(),
  m_locator(),
  m_status(),
  m_source_type(),
  m_file_info(),
  m_url_info(),
  m_image_info(),
  m_archive_info(),
  m_children()
{}

ResourceInfo::ResourceInfo(const RowId& id, const ResourceInfo& other) :
  m_id(id),
  m_name(other.m_name),
  m_locator(other.m_locator),
  m_status(other.m_status),
  m_source_type(other.m_source_type),
  m_file_info(other.m_file_info),
  m_url_info(other.m_url_info),
  m_image_info(other.m_image_info),
  m_archive_info(other.m_archive_info),
  m_children(other.m_children)
{}

ResourceInfo::ResourceInfo(const RowId& id,
                           const ResourceName& name,
                           ResourceStatus status) :
  m_id(id),
  m_name(name),
  m_locator(),
  m_status(status),
  m_source_type(),
  m_file_info(),
  m_url_info(),
  m_image_info(),
  m_archive_info(),
  m_children()
{}

ResourceInfo::ResourceInfo(const ResourceName& name,
                           ResourceStatus status) :
  m_id(),
  m_name(name),
  m_locator(),
  m_status(status),
  m_source_type(),
  m_file_info(),
  m_url_info(),
  m_image_info(),
  m_archive_info(),
  m_children()
{}

SourceType
ResourceInfo::get_source_type() const
{
  return m_source_type;
}

URLInfo
ResourceInfo::get_url_info() const
{
  assert(m_source_type == SourceType::URL);
  return m_url_info;
}

FileInfo
ResourceInfo::get_file_info() const
{
  assert(m_source_type == SourceType::File);
  return m_file_info;
}

    
ResourceType
ResourceInfo::get_type() const
{
  return ResourceType_from_string(m_name.get_handler().get_type());
}

ImageInfo
ResourceInfo::get_image_info() const
{
  assert(get_type() == ResourceType::Image);

  return m_image_info;
}

ArchiveInfo
ResourceInfo::get_archive_info() const
{
  assert(get_type() == ResourceType::Archive);
  
  return m_archive_info;
}

std::vector<ResourceInfo>
ResourceInfo::get_children() const
{
  return m_children;
}

/* EOF */
