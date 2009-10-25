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

#include "plugins/pnm.hpp"

#include <stdexcept>

#include "plugins/pnm_mem_reader.hpp"
#include "math/size.hpp"

SoftwareSurfacePtr
PNM::load_from_mem(const char* data, int len)
{
  PNMMemReader pnm(data, len);

  SoftwareSurfacePtr surface = SoftwareSurface::create(SoftwareSurface::RGB_FORMAT, pnm.get_size());
  const uint8_t* src_pixels = (uint8_t*)pnm.get_pixel_data();
  uint8_t* dst_pixels = surface->get_data();
  //std::cout << "MaxVal: " << pnm.get_maxval() << std::endl;
  assert(pnm.get_maxval() == 255);
  if (pnm.get_magic() == "P6") // RGB
  {
    for(int i = 0; i < surface->get_width() * surface->get_height(); ++i)
    {
      dst_pixels[3*i+0] = src_pixels[3*i+0];
      dst_pixels[3*i+1] = src_pixels[3*i+1];
      dst_pixels[3*i+2] = src_pixels[3*i+2];
    }
  }
  else if (pnm.get_magic() == "P5") // Grayscale
  {
    for(int i = 0; i < surface->get_width() * surface->get_height(); ++i)
    {
      dst_pixels[3*i+0] = src_pixels[i];
      dst_pixels[3*i+1] = src_pixels[i];
      dst_pixels[3*i+2] = src_pixels[i];
    }
  }
  else
  {
    throw std::runtime_error("Unhandled PNM format: '" + pnm.get_magic() + "'");
  }

  return surface;
}

/* EOF */
