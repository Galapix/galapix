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

#include <assert.h>
#include <iostream>
#include <stdexcept>

#include "blob.hpp"
#include "math.hpp"
#include "jpeg.hpp"
#include "math/rect.hpp"
#include "math/size.hpp"

#include "url.hpp"
#include "software_surface.hpp"

class SoftwareSurfaceImpl
{
public:
  Size     size;
  int      pitch;
  uint8_t* pixels;
  
  SoftwareSurfaceImpl(const Size& size)
    : size(size),
      pitch(size.width * 3),
      pixels(new uint8_t[pitch * size.height])
  {
  }

  ~SoftwareSurfaceImpl() 
  {
    delete[] pixels;
  }
};

SoftwareSurface::SoftwareSurface()
{
}

SoftwareSurface::SoftwareSurface(const Size& size)
 : impl(new SoftwareSurfaceImpl(size))
{
}

SoftwareSurface::~SoftwareSurface()
{
}

SoftwareSurface
SoftwareSurface::scale(const Size& size) const
{
  return SoftwareSurface();
}

SoftwareSurface
SoftwareSurface::crop(const Rect& rect) const
{
  return SoftwareSurface();
}

Size
SoftwareSurface::get_size()  const
{
  return impl->size;
}

int
SoftwareSurface::get_width()  const
{
  return impl->size.width;
}

int
SoftwareSurface::get_height() const
{
  return impl->size.height;
}

int
SoftwareSurface::get_pitch()  const
{
  return impl->pitch;
}

void
SoftwareSurface::save(const std::string& filename) const
{
  assert(!"SoftwareSurface::save(const std::string& filename) const");
}

Blob
SoftwareSurface::get_jpeg_data() const
{
  assert(!"SoftwareSurface::get_jpeg_data() const");
  return Blob();
}

SoftwareSurface
SoftwareSurface::from_data(const Blob& blob)
{
  return JPEG::load(blob.get_data(), blob.size());
}

uint8_t*
SoftwareSurface::get_data() const
{
  return impl->pixels;
}

uint8_t*
SoftwareSurface::get_row_data(int y) const
{
  return impl->pixels + (y * impl->pitch);
  
}
  
/* EOF */
