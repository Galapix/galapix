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

// Basic program that takes a black&white image and tries to fill in
// missing pixels

#include <iostream>
#include <assert.h>

#include <surf/software_surface.hpp>
#include <surf/software_surface_factory.hpp>
#include <surf/color.hpp>

#include "plugins/png.hpp"
#include "plugins/jpeg.hpp"
#include "util/url.hpp"

using namespace surf;

namespace {

bool is_black(const RGB8Pixel rgb)
{
  return (rgb.r < 128 ||
          rgb.g < 128 ||
          rgb.b < 128);
}

RGB8Pixel blackfill_pixel(RGB8Pixel p00, RGB8Pixel p10, RGB8Pixel p20,
                          RGB8Pixel p01, RGB8Pixel p11, RGB8Pixel p21,
                          RGB8Pixel p02, RGB8Pixel p12, RGB8Pixel p22)
{
  if (
    // horz/vert
    (is_black(p01) && is_black(p21)) ||
    (is_black(p10) && is_black(p12)) ||

    // diagonal
    (is_black(p00) && is_black(p22)) ||
    (is_black(p20) && is_black(p02)) ||

    // half diag left
    (is_black(p01) && is_black(p20)) ||
    (is_black(p01) && is_black(p22)) ||

    // half diag right
    (is_black(p00) && is_black(p21)) ||
    (is_black(p02) && is_black(p21)) ||

    // half diag top
    (is_black(p10) && is_black(p02)) ||
    (is_black(p10) && is_black(p22)) ||

    // half diag bottom
    (is_black(p12) && is_black(p00)) ||
    (is_black(p12) && is_black(p20))
    )
  {
    return RGB8Pixel(0, 0, 0);
  }
  else
  {
    return p11;
  }
}

RGB8Pixel get_pixel(PixelView<RGB8Pixel> const& img, int x, int y)
{
  RGB8Pixel rgb = img.get_pixel({x, y});
  return rgb;
}

void blackfill(PixelView<RGB8Pixel> const& in,
               PixelView<RGB8Pixel>& out)
{
  for(int y = 0; y < in.get_height()-2; ++y) {
    for(int x = 0; x < in.get_width()-2; ++x) {
      RGB8Pixel p00 = get_pixel(in, x+0, y+0);
      RGB8Pixel p10 = get_pixel(in, x+1, y+0);
      RGB8Pixel p20 = get_pixel(in, x+2, y+0);

      RGB8Pixel p01 = get_pixel(in, x+0, y+1);
      RGB8Pixel p11 = get_pixel(in, x+1, y+1);
      RGB8Pixel p21 = get_pixel(in, x+2, y+1);

      RGB8Pixel p02 = get_pixel(in, x+0, y+2);
      RGB8Pixel p12 = get_pixel(in, x+1, y+2);
      RGB8Pixel p22 = get_pixel(in, x+2, y+2);

      out.put_pixel({x+1, y+1},
                    blackfill_pixel(p00, p10, p20,
                                    p01, p11, p21,
                                    p02, p12, p22));
    }
  }
}

} // namespace

int main(int argc, char* argv[])
{
  assert(argc != 2);
  SoftwareSurfaceFactory factory;

  for(int i = 1; i < argc; ++i)
  {
    std::cout << "Loading: " << argv[i] << std::endl;
    SoftwareSurface in  = factory.from_file(argv[1]);
    SoftwareSurface out = SoftwareSurface::create(surf::PixelFormat::RGB8, in.get_size());

    blackfill(in.as_pixelview<RGB8Pixel>(), out.as_pixelview<RGB8Pixel>());

    //png::save(out, argv[2]);
    jpeg::save(out, argv[2], 85);
  }

  return 0;
}

/* EOF */
