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

#ifndef HEADER_GALAPIX_RESOURCE_IMAGE_INFO_HPP
#define HEADER_GALAPIX_RESOURCE_IMAGE_INFO_HPP

#include "database/row_id.hpp"

class ImageInfo
{
private:
  RowId m_id;
  RowId m_resource_id;
  int m_width;
  int m_height;

public:
  ImageInfo() :
    m_id(),
    m_resource_id(),
    m_width(),
    m_height()
  {}

  ImageInfo(int width, int height) :
    m_id(),
    m_resource_id(),
    m_width(width),
    m_height(height)
  {}

  ImageInfo(const RowId& id, const RowId& resource_id, int width, int height) :
    m_id(id),
    m_resource_id(resource_id),
    m_width(width),
    m_height(height)
  {}

  ImageInfo(const RowId& id, const ImageInfo& image_info) :
    m_id(id),
    m_resource_id(image_info.m_resource_id),
    m_width(image_info.m_width),
    m_height(image_info.m_height)
  {}

  RowId get_id() const { return m_id; }
  RowId get_resource_id() const { return m_resource_id; }
  int get_width() const { return m_width; }
  int get_height() const { return m_height; }
};

#endif

/* EOF */
