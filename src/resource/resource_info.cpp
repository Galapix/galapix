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

#include "resource_info.hpp"

ResourceInfo
ResourceInfo::from_locator(const ResourceLocator& locator)
{
  ResourceInfo info;
  
  info.m_locator = locator;
  // info.m_name = get sha1 from locator.url, run magic to detect type
  // info.m_children = if archive, gather filenames in archive

  return info;
}

ResourceInfo::ResourceInfo() :
  m_locator(),
  m_name(),
  m_children()
{
}

/* EOF */
