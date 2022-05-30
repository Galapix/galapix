// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_RESOURCE_RESOURCE_INFO_HPP
#define HEADER_GALAPIX_RESOURCE_RESOURCE_INFO_HPP

#include <string>
#include <vector>
#include <stdexcept>

#include "database/row_id.hpp"
#include "resource/resource_handler.hpp"
#include "resource/resource_name.hpp"
#include "resource/resource_status.hpp"
#include "resource/resource_type.hpp"

namespace galapix {

/** ResourceInfo provides access to all the meta data related to a
    given ResourceLocator, such as the name, status */
class ResourceInfo
{
public:
  ResourceInfo();
  ResourceInfo(RowId const& id, ResourceInfo const& other);
  ResourceInfo(RowId const& id, ResourceName const& name, ResourceStatus status);
  ResourceInfo(ResourceName const& name, ResourceStatus status);

  RowId get_id() const { return m_id; }

  ResourceName get_name() const { return m_name; }
  ResourceStatus get_status() const { return m_status; }

  BlobInfo get_blob_info() const { return m_name.get_blob_info(); }
  ResourceHandler get_handler() const { return m_name.get_handler(); }

  ResourceType get_type() const;

private:
  RowId m_id;
  ResourceName    m_name;
  ResourceStatus  m_status;
};

} // namespace galapix

#endif

/* EOF */
