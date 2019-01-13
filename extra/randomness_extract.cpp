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
#include <sstream>
#include <math.h>

#include "math/math.hpp"
#include "util/url.hpp"
#include "util/software_surface_factory.hpp"
#include "util/software_surface.hpp"
#include "util/software_surface_float.hpp"
#include "plugins/jpeg.hpp"
#include "plugins/png.hpp"

int main(int argc, char** argv)
{
  if (argc == 1)
  {
    std::cout << "Usage: " << argv[0] << " FILES" << std::endl;
    std::cout << "Converts an image to black&white by using the least significant bit" << std::endl;
  }
  else
  {
    SoftwareSurfaceFactory factory;

    for(int i = 1; i < argc; ++i)
    {
      std::cout << "Loading: " << argv[i] << std::endl;

      SoftwareSurface image = factory.from_url(URL::from_string(argv[i]));
      PixelData const& src = image.get_pixel_data();
      PixelData out = PixelData(PixelData::RGB_FORMAT, src.get_size());

      std::cout << "Processing image..." << std::endl;
      for(int y = 0; y < out.get_height(); ++y)
        for(int x = 0; x < out.get_width(); ++x)
        {
          RGB rgb;
          src.get_pixel(x, y, rgb);

          if (rgb.r & (1<<3))
            rgb.r = 0;
          else
            rgb.r = 255;

          if (rgb.g & (1<<3))
            rgb.g = 0;
          else
            rgb.g = 255;

          if (rgb.b & (1<<3))
            rgb.b = 0;
          else
            rgb.b = 255;

          out.put_pixel(x, y, rgb);
        }
      std::cout << "Processing image... Done" << std::endl;

      std::ostringstream out_filename;
      out_filename << "/tmp/out" << i << ".png";
      PNG::save(out, out_filename.str());

      std::cout << "Wrote " << out_filename.str() << std::endl;
      std::cout << "Done" << std::endl;
    }
  }

  return 0;
}

/* EOF */
