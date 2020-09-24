/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2011 Ingo Ruhnke <grumbel@gmail.com>
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

// Little toy application that takes an image and then overlays it
// with a checkboard pattern of it's inverted image. This leads to a
// grey looking image, unless the display gamma is changed.

#include "plugins/png.hpp"
#include "surface/software_surface_float.hpp"
#include "surface/software_surface.hpp"
#include "surface/software_surface_factory.hpp"

using namespace surf;

namespace {

void invert(SoftwareSurfaceFloatPtr surface)
{
  for(int y = 0; y < surface->get_height(); ++y)
  {
    for(int x = y%2; x < surface->get_width(); x+=2)
    {
      RGBAf rgba;
      surface->get_pixel(x, y, rgba);
      rgba.r = 1.0f - rgba.r;
      rgba.g = 1.0f - rgba.g;
      rgba.b = 1.0f - rgba.b;
      surface->put_pixel(x, y, rgba);
    }
  }
}

} // namespace

int main(int argc, char** argv)
{
  SoftwareSurfaceFactory software_surface_factory;

  float gamma = 2.2f;

  for(int i = 1; i < argc; ++i)
  {
    SoftwareSurface surface = software_surface_factory.from_file(argv[i]);
    SoftwareSurfaceFloatPtr surfacef = SoftwareSurfaceFloat::from_software_surface(surface);

    surfacef->apply_gamma(gamma);
    invert(surfacef);
    surfacef->apply_gamma(1.0f/gamma);

    surface = surfacef->to_software_surface();
    PNG::save(surface, "/tmp/out.png");
  }

  return 0;
}

/* EOF */
