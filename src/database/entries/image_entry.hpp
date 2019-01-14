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

#ifndef HEADER_GALAPIX_DATABASE_ENTRIES_IMAGE_ENTRY_HPP
#define HEADER_GALAPIX_DATABASE_ENTRIES_IMAGE_ENTRY_HPP

#include "math/math.hpp"
#include "math/size.hpp"
#include "database/row_id.hpp"

class ImageEntry final
{
public:
  ImageEntry() :
    m_id(),
    m_resource_id(),
    m_size(),
    m_max_scale()
  {}

  ImageEntry(const RowId& id, const RowId& resource_id, int width, int height) :
    m_id(id),
    m_resource_id(resource_id),
    m_size(width, height),
    m_max_scale()
  {
    int s = Math::max(width, height);
    m_max_scale = 0;
    while(s > 8)
    {
      s /= 2;
      m_max_scale += 1;
    }
  }

  RowId  get_resource_id() const { return m_resource_id; }
  int    get_width()   const { return m_size.width; }
  int    get_height()  const { return m_size.height; }
  Size   get_size()    const { return m_size; }
  int    get_max_scale() const { return m_max_scale; }

private:
  RowId m_id;

  RowId m_resource_id;

  /** The size of the image in pixels */
  Size m_size;

  int m_max_scale;
};

#endif

/* EOF */
