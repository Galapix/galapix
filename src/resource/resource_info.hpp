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

#ifndef HEADER_GALAPIX_GALAPIX_RESOURCE_INFO_HPP
#define HEADER_GALAPIX_GALAPIX_RESOURCE_INFO_HPP

#include <vector>

#include "resource/resource_locator.hpp"
#include "resource/resource_name.hpp"

class ResourceInfo
{
private:
  ResourceLocator m_locator;
  ResourceName m_name;
  std::vector<ResourceLocator> m_children;

public:
  static ResourceInfo from_locator(const ResourceLocator& locator);
  
public:
  ResourceInfo();

  ResourceLocator get_locator() const { return m_locator; }
  ResourceName get_name() const { return m_name; }
  std::vector<ResourceLocator> get_children() const { return m_children; }
};

#endif

/* EOF */
