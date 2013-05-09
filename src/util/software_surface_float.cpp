/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmx.de>
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

#include "util/software_surface_float.hpp"

SoftwareSurfaceFloatPtr 
SoftwareSurfaceFloat::create(const Size& size)
{
  return SoftwareSurfaceFloatPtr(new SoftwareSurfaceFloat(size));
}

SoftwareSurfaceFloat::SoftwareSurfaceFloat(const Size& size) :
  m_size(size),
  m_pixels(size.get_area() * 4)
{
}

Size
SoftwareSurfaceFloat::get_size() const
{
  return m_size;
}

int
SoftwareSurfaceFloat::get_width() const
{
  return m_size.width;
}

int
SoftwareSurfaceFloat::get_height() const
{
  return m_size.height;
}

int
SoftwareSurfaceFloat::get_pitch() const
{
  return m_size.width * 4;
}

void
SoftwareSurfaceFloat::put_pixel(int x, int y, const RGBAf& rgba)
{
  m_pixels[get_pitch() * y + 4*x + 0] = rgba.r;
  m_pixels[get_pitch() * y + 4*x + 1] = rgba.g;
  m_pixels[get_pitch() * y + 4*x + 2] = rgba.b;
  m_pixels[get_pitch() * y + 4*x + 3] = rgba.a;
}

void
SoftwareSurfaceFloat::get_pixel(int x, int y, RGBAf& rgba) const
{
  rgba.r = m_pixels[get_pitch() * y + 4*x + 0];
  rgba.g = m_pixels[get_pitch() * y + 4*x + 1];
  rgba.b = m_pixels[get_pitch() * y + 4*x + 2];
  rgba.a = m_pixels[get_pitch() * y + 4*x + 3];
}

/* EOF */
