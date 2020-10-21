/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2010 Ingo Ruhnke <grumbel@gmail.com>
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

#include <iostream>
#include <math.h>

#include <surf/software_surface_factory.hpp>
#include <surf/software_surface.hpp>
#include <surf/software_surface_float.hpp>

#include "math/math.hpp"
#include "util/url.hpp"
#include "plugins/jpeg.hpp"

using namespace surf;

int main(int argc, char** argv)
{
  SoftwareSurfaceFactory factory;

  for(int i = 1; i < argc; ++i)
  {
    SoftwareSurface image = factory.from_file(argv[i]);

    std::string outfile = argv[i];
    outfile += "cyber.JPG";

    image = image.halve();

    PixelData const& src = image.get_pixel_data();
    PixelData out(surf::PixelFormat::RGB, src.get_size());

    for(int y = 0; y < src.get_height(); ++y)
    {
      uint8_t const* ipixels = src.get_row_data(y);
      uint8_t* opixels = out.get_row_data(y);

      if (y % 2 == 0)
      {
        for(int x = 0; x < src.get_width()*3; x += 3)
        {
          opixels[x+0] = ipixels[x+0];
          opixels[x+1] = ipixels[x+0];
          opixels[x+2] = ipixels[x+0];
        }
      }
      else
      {
        for(int x = 0; x < src.get_width()*3; x += 3)
        {
          opixels[x+0] = 0;
          opixels[x+1] = ipixels[x+1];
          opixels[x+2] = ipixels[x+2];
        }
      }
    }

    JPEG::save(out,  100, outfile);
    std::cout << "Wrote: " << outfile << std::endl;
  }

  return 0;
}

  /* EOF */
