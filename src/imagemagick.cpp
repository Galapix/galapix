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
#include <stdexcept>
#include <Magick++.h>
#include <iostream>
#include "math/size.hpp"
#include "imagemagick.hpp"

bool
Imagemagick::get_size(const std::string& filename, Size& size)
{
  try {
    Magick::Image image(filename);
  
    size.width  = image.columns();
    size.height = image.rows();

    return true;
  } catch(std::exception& err) {
    std::cout << "Imagemagick: " << filename << ": " << err.what() << std::endl;
    return false;
  }
}

SoftwareSurface
Imagemagick::load_from_file(const std::string& filename)
{
  Magick::Image image(filename);

  SoftwareSurface surface;

  int width  = image.columns();
  int height = image.rows();


  int shift;
  if (MaxRGB == 65535)
    shift = 8;
  else if (MaxRGB == 255)
    shift = 0;
  else
    assert(!"Imagemagick: Unknown MaxRGB");

  if (image.matte())
    {
      surface = SoftwareSurface(SoftwareSurface::RGBA_FORMAT, 
                                Size(width, height));

      for(int y = 0; y < height; ++y)
        {
          const Magick::PixelPacket* src_pixels = image.getConstPixels(0, y, width, 1);
          uint8_t* dst_pixels = surface.get_row_data(y);

          for(int x = 0; x < width; ++x)
            {
              dst_pixels[4*x + 0] = src_pixels[x].red     >> shift;
              dst_pixels[4*x + 1] = src_pixels[x].green   >> shift;
              dst_pixels[4*x + 2] = src_pixels[x].blue    >> shift;
              dst_pixels[4*x + 3] = src_pixels[x].opacity >> shift;
            }
        }
    }
  else
    {
      surface = SoftwareSurface(SoftwareSurface::RGB_FORMAT, 
                                Size(width, height));

      for(int y = 0; y < height; ++y)
        {
          std::cout << y << "/" << height << std::endl;

          const Magick::PixelPacket* src_pixels = image.getConstPixels(0, y, width, 1);
          uint8_t* dst_pixels = surface.get_row_data(y);

          for(int x = 0; x < width; ++x)
            {
              dst_pixels[3*x + 0] = src_pixels[x].red     >> shift;
              dst_pixels[3*x + 1] = src_pixels[x].green   >> shift;
              dst_pixels[3*x + 2] = src_pixels[x].blue    >> shift;
              dst_pixels[3*x + 3] = src_pixels[x].opacity >> shift;
            }
        }
    }  

  return surface;
}

/* EOF */