/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
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
