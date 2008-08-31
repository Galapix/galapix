/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_JPEG_IMAGE_HPP
#define HEADER_JPEG_IMAGE_HPP

#include <string>
#include "math/size.hpp"
#include "math/rect.hpp"
#include "software_surface.hpp"
#include "blob.hpp"

class JPEGImageImpl;

class JPEGImage
{
public:
  JPEGImage();
  JPEGImage(void* data, int len);
  JPEGImage(const std::string& filename);

  /** Crops the JPEG, rect coordinates must be aligned to 8px */
  JPEGImage crop(const Rect& rect);

  /** Return the compressed JPEG data */
  Blob get_data() const;

  int  get_width()  const;
  int  get_height() const;
  Size get_size()   const;

  SoftwareSurface create_thumbnail(int scale);

private:
  boost::shared_ptr<JPEGImageImpl> impl;
};

#endif

/* EOF */
