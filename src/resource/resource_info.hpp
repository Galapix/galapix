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
#include "resource/archive_info.hpp"
#include "resource/blob_info.hpp"
#include "resource/file_info.hpp"
#include "resource/file_info.hpp"
#include "resource/image_info.hpp"
#include "resource/resource_handler.hpp"
#include "resource/resource_locator.hpp"
#include "resource/resource_name.hpp"
#include "resource/resource_status.hpp"
#include "resource/resource_type.hpp"
#include "resource/source_info.hpp"
#include "resource/url_info.hpp"
#include "util/sha1.hpp"

/**
    ResourceInfo provides access to all the meta data related to a
    given ResourceLocator, such as the name, status
*/
class ResourceInfo
{
private:
  RowId m_id;
  ResourceName    m_name;
  ResourceLocator m_locator;
  ResourceStatus  m_status;

  SourceType m_source_type;
  FileInfo m_file_info;
  URLInfo m_url_info;

  ImageInfo m_image_info;
  ArchiveInfo m_archive_info;

  std::vector<ResourceInfo> m_children;

public:
  ResourceInfo();
  ResourceInfo(const RowId& id, const ResourceInfo& other);
  ResourceInfo(const RowId& id, const ResourceName& name, ResourceStatus status);
  ResourceInfo(const ResourceName& name, ResourceStatus status);

  RowId get_id() const { return m_id; }

  ResourceName get_name() const { return m_name; }
  ResourceLocator get_locator() const { return m_locator; }

  ResourceStatus get_status() const { return m_status; }

  BlobInfo get_blob_info() const { return m_name.get_blob_info(); }
  ResourceHandler get_handler() const { return m_name.get_handler(); }

  SourceType get_source_type() const;
  URLInfo    get_url_info() const;
  FileInfo   get_file_info() const;

  ResourceType get_type() const;
  ImageInfo    get_image_info() const;
  ArchiveInfo  get_archive_info() const;

  std::vector<ResourceInfo> get_children() const;
};

#endif

/* EOF */
