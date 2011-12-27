/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmx.de>
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

#include "plugins/png.hpp"
#include "util/software_surface_float.hpp"
#include "util/software_surface.hpp"
#include "util/software_surface_factory.hpp"

void scanlines(SoftwareSurfaceFloatPtr surface)
{
  for(int y = 0; y < surface->get_height(); ++y)
  {
    for(int x = 0; x < surface->get_width(); ++x)
    {
      RGBAf rgba;
      surface->get_pixel(x, y, rgba);
      
      int v = (x+y%3) % 3;
      if (v % 6 == 0)
      {      
        rgba.r = rgba.r * 3.0f;
        rgba.g = 0.0f;
        rgba.b = 0.0f;
      }
      else if (v % 6 == 1)
      {
        rgba.r = 0.0f;
        rgba.g = rgba.g * 3.0f;
        rgba.b = 0.0f;
      }
      else if (v % 6 == 2)
      {
        rgba.r = 0.0f;
        rgba.g = 0.0f;
        rgba.b = rgba.b * 3.0f;
      }

      rgba.clamp();
      
      surface->put_pixel(x, y, rgba);
    }
  }
}

int main(int argc, char** argv)
{
  SoftwareSurfaceFactory software_surface_factory;
  
  float gamma = 2.2f;
  for(int i = 1; i < argc; ++i)
  {
    SoftwareSurfacePtr surface = software_surface_factory.from_file(argv[i]);
    SoftwareSurfaceFloatPtr surfacef = SoftwareSurfaceFloat::from_software_surface(surface);

    surfacef->apply_gamma(gamma);
    scanlines(surfacef);
    surfacef->apply_gamma(1.0f/gamma);

    surface = surfacef->to_software_surface();
    PNG::save(surface, "/tmp/out.png");
  }

  return 0;
}

/* EOF */
