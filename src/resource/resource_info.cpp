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

#include "resource/resource_info.hpp"

ResourceInfo::ResourceInfo() :
  m_id(),
  m_name(),
  m_status()
{}

ResourceInfo::ResourceInfo(const RowId& id, const ResourceInfo& other) :
  m_id(id),
  m_name(other.m_name),
  m_status(other.m_status)
{}

ResourceInfo::ResourceInfo(const RowId& id,
                           const ResourceName& name,
                           ResourceStatus status) :
  m_id(id),
  m_name(name),
  m_status(status)
{}

ResourceInfo::ResourceInfo(const ResourceName& name,
                           ResourceStatus status) :
  m_id(),
  m_name(name),
  m_status(status)
{}

ResourceType
ResourceInfo::get_type() const
{
  return ResourceType_from_string(m_name.get_handler().get_type());
}

/* EOF */
