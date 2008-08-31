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

#include "jpeg_image.hpp"

class JPEGImageImpl
{
public:
  Size size;
};

JPEGImage::JPEGImage()
{
}

JPEGImage::JPEGImage(void* data, int len)
{
}

JPEGImage::JPEGImage(const std::string& filename)
{
}

JPEGImage
JPEGImage::crop(const Rect& rect)
{
  return JPEGImage();
}

Blob
JPEGImage::get_data() const
{
  return Blob();
}

SoftwareSurface
JPEGImage::create_thumbnail(int scale)
{
  return SoftwareSurface();
}

int
JPEGImage::get_width() const
{
  if (impl.get())
    return impl->size.width;
  else
    return 0;
}

int
JPEGImage::get_height() const
{
  if (impl.get())
    return impl->size.height;
  else
    return 0;
}

Size
JPEGImage::get_size() const
{
  if (impl.get())
    return impl->size;
  else
    return Size(0, 0);
}

/* EOF */
