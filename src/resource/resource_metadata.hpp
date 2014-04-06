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

#ifndef HEADER_GALAPIX_RESOURCE_RESOURCE_DATA_HPP
#define HEADER_GALAPIX_RESOURCE_RESOURCE_DATA_HPP

#include "resource/archive_info.hpp"
#include "resource/blob_info.hpp"
#include "resource/file_info.hpp"
#include "resource/file_info.hpp"
#include "resource/image_info.hpp"
#include "resource/resource_info.hpp"
#include "resource/resource_locator.hpp"
#include "resource/resource_locator.hpp"
#include "resource/source_info.hpp"
#include "resource/url_info.hpp"
#include "util/sha1.hpp"

class ResourceMetadata
{
private:
  ResourceInfo m_info;
  ResourceLocator m_locator;

  SourceType m_source_type;
  FileInfo m_file_info;
  URLInfo m_url_info;

  ImageInfo m_image_info;
  ArchiveInfo m_archive_info;

  std::vector<ResourceName> m_children;

public:
  ResourceMetadata() :
    m_info(),
    m_locator(),
    m_source_type(),
    m_file_info(),
    m_url_info(),
    m_image_info(),
    m_archive_info(),
    m_children()
  {}

  ResourceInfo get_info() const
  {
    return m_info;
  }

  ResourceLocator get_locator() const { return m_locator; }

  SourceType get_source_type() const;
  URLInfo    get_url_info() const;
  FileInfo   get_file_info() const;

  ImageInfo    get_image_info() const;
  ArchiveInfo  get_archive_info() const;

  ResourceType get_type() const;

  std::vector<ResourceName> get_children() const;

private:
  ResourceMetadata(const ResourceMetadata&) = delete;
  ResourceMetadata& operator=(const ResourceMetadata&) = delete;
};

#endif

/* EOF */
