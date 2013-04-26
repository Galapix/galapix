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

#include <string>
#include <vector>
#include <stdexcept>

#include "database/row_id.hpp"
#include "resource/resource_handler.hpp"
#include "resource/resource_locator.hpp"
#include "resource/resource_name.hpp"
#include "resource/resource_status.hpp"
#include "util/sha1.hpp"

class ResourceInfo
{
private:
  RowId m_id;
  SHA1 m_sha1;
  ResourceHandler m_handler;
  ResourceStatus m_status;
  
public:
  ResourceInfo() :
    m_id(),
    m_sha1(),
    m_handler(),
    m_status()
  {}

  ResourceInfo(const RowId& id, const ResourceInfo& other) :
    m_id(id),
    m_sha1(other.m_sha1),
    m_handler(other.m_handler),
    m_status(other.m_status)
  {}

  ResourceInfo(const RowId& id,
               const SHA1& sha1,
               const ResourceHandler& handler,
               ResourceStatus status) :
    m_id(id),
    m_sha1(sha1),
    m_handler(handler),
    m_status(status)
  {}

  RowId get_id() const { return m_id; }
  const SHA1& get_sha1() const { return m_sha1; }
  ResourceHandler get_handler() const { return m_handler; }
  ResourceStatus get_status() const { return m_status; }

  //ResourceLocator get_locator() const { return m_locator; }
  //ResourceName get_name() const { return m_name; }
  //std::vector<ResourceLocator> get_children() const { return m_children; }
};

#endif

/* EOF */
