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

#include "math/math.hpp"
#include "util/url.hpp"
#include "plugins/jpeg.hpp"

using namespace surf;

namespace {

void add(SoftwareSurface& dst, SoftwareSurface const& src)
{
  for(int y = 0; y < dst.get_height(); ++y) {
    for(int x = 0; x < dst.get_width(); ++x) {
      Color rgb = src.get_pixel({x, y});
      Color rgba = dst.get_pixel({x, y});

      rgba.r += powf(rgb.r, 0.4545f);
      rgba.g += powf(rgb.g, 0.4545f);
      rgba.b += powf(rgb.b, 0.4545f);

      dst.put_pixel({x, y}, rgba);
    }
  }
}

void tone_map(SoftwareSurface& out, SoftwareSurface const& in, float factor)
{
  for(int y = 0; y < out.get_height(); ++y) {
    for(int x = 0; x < out.get_width(); ++x) {
      Color rgba = in.get_pixel({x, y});
      Color rgb;
      rgb.r = std::clamp(powf(rgba.r / factor, 2.2f), 0.0f, 1.0f);
      rgb.g = std::clamp(powf(rgba.g / factor, 2.2f), 0.0f, 1.0f);
      rgb.b = std::clamp(powf(rgba.b / factor, 2.2f), 0.0f, 1.0f);
      out.put_pixel({x, y}, rgb);
    }
  }
}

} // namespace

int main(int argc, char** argv)
{
  if (argc == 1)
  {
    std::cout << "Usage: " << argv[0] << " FILES" << std::endl;
    std::cout << "Simple tool to build an average out of multiple images." << std::endl;
  }
  else
  {
    SoftwareSurfaceFactory factory;

    SoftwareSurface out;
    for(int i = 1; i < argc; ++i)
    {
      std::cout << "Loading: " << argv[i] << std::endl;
      SoftwareSurface image = factory.from_file(argv[i]);
      add(out, image);
    }

    SoftwareSurface out_rgb = SoftwareSurface::create(surf::PixelFormat::RGB8, out.get_size());
    tone_map(out_rgb, out, static_cast<float>(argc - 1));

    std::string out_filename = "/tmp/out.jpg";
    jpeg::save(out_rgb, out_filename, 100);
    std::cout << "Wrote " << out_filename << std::endl;
    std::cout << "Done" << std::endl;
  }

  return 0;
}

/* EOF */
