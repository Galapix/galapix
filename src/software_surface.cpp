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

void
SoftwareSurface::put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b)
{
  assert(x >= 0 && x < impl->size.width &&
         y >= 0 && y < impl->size.height);

  impl->pixels[y * impl->pitch + x*3 + 0] = r;
  impl->pixels[y * impl->pitch + x*3 + 1] = g;
  impl->pixels[y * impl->pitch + x*3 + 2] = b;
}

void
SoftwareSurface::get_pixel(int x, int y, uint8_t* r, uint8_t* g, uint8_t* b) const
{
  assert(x >= 0 && x < impl->size.width &&
         y >= 0 && y < impl->size.height);

  *r = impl->pixels[y * impl->pitch + x*3 + 0];
  *g = impl->pixels[y * impl->pitch + x*3 + 1];
  *b = impl->pixels[y * impl->pitch + x*3 + 2];
}

SoftwareSurface
SoftwareSurface::halve() const
{
  return scale(Size(get_width()/2, get_height()/2));
}

SoftwareSurface
SoftwareSurface::scale(const Size& size) const
{
  SoftwareSurface surface(size);
  // Very much non-fast, needs replacement with proper

  uint8_t r,g,b;
  for(int y = 0; y < surface.get_height(); ++y)
    for(int x = 0; x < surface.get_width(); ++x)
      {
        get_pixel(x * impl->size.width  / surface.impl->size.width,
                  y * impl->size.height / surface.impl->size.height,
                  &r, &g, &b);

        surface.put_pixel(x, y, r, g, b);
      }

  return surface;
}

SoftwareSurface
SoftwareSurface::crop(const Rect& rect_in) const
{
  // FIXME: We could do a crop without copying contain, simply
  // reference the old SoftwareSurfaceImpl and have a different pitch
  // and pixel offset
  assert(rect_in.is_normal());
 
  // Clip the rectangle to the image
  Rect rect(Math::max(0, rect_in.left),
            Math::max(0, rect_in.top),
            Math::min(get_width(),  rect_in.right), 
            Math::min(get_height(), rect_in.bottom));

  SoftwareSurface surface(rect.get_size());

  for(int y = rect.top; y < rect.bottom; ++y)
    {
      memcpy(surface.get_row_data(y - rect.top), 
             get_row_data(y) + rect.left*3,
             rect.get_width() * 3);
    }

  return surface;
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
  return JPEG::save(*this, 75);
}

SoftwareSurface
SoftwareSurface::from_file(const std::string& filename)
{
  return JPEG::load_from_file(filename);
}

SoftwareSurface
SoftwareSurface::from_data(const Blob& blob)
{
  return JPEG::load_from_mem(blob.get_data(), blob.size());
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
