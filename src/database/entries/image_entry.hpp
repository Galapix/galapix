/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_DATABASE_IMAGE_ENTRY_HPP
#define HEADER_GALAPIX_DATABASE_IMAGE_ENTRY_HPP

#include "math/math.hpp"
#include "math/size.hpp"
#include "database/row_id.hpp"

class ImageEntry final
{
public:
  enum Handler 
  {
    kUnknownHandler,
    kJPEGHandler,
    kPNGHandler,
    kRSVGHandler,
    kKRAHandler,
    kUFRawHandler,
    kVidThumbHandler,
    kDDSHandler,
    kImagemagickHandler
  };

private:
  RowId m_id;

  RowId m_blob_id;

  /** The size of the image in pixels */
  Size m_size;

  /** The handler of the image (RGB, RGBA) */
  int  m_handler;

  int m_max_scale;

public:
  ImageEntry() :
    m_id(),
    m_blob_id(),
    m_size(),
    m_handler(kUnknownHandler),
    m_max_scale()
  {}

  ImageEntry(const RowId& id, const RowId& blob_id, int width, int height, int handler) :
    m_id(id),
    m_blob_id(blob_id),
    m_size(width, height),
    m_handler(handler),
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

  RowId  get_blob_id() const { return m_blob_id; }
  int    get_width()   const { return m_size.width; }
  int    get_height()  const { return m_size.height; }
  Size   get_size()    const { return m_size; }
  int    get_handler()  const { return m_handler; }
  int    get_max_scale() const { return m_max_scale; }
};

#endif

/* EOF */
