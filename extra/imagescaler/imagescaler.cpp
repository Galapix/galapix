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

#include <iostream>
#include <sstream>
#include <limits>

#include <surf/software_surface_factory.hpp>
#include <surf/software_surface.hpp>
#include <surf/transform.hpp>
#include <surf/color.hpp>

#include "math/size.hpp"
#include "math/vector2i.hpp"
#include "math/rect.hpp"
#include <surf/plugins/png.hpp>

using namespace surf;
using namespace galapix;

namespace {

struct Pixel
{
  uint8_t tl;
  uint8_t tr;
  uint8_t bl;
  uint8_t br;
};

struct TileDBEntry
{
  Pixel pixels;
  SoftwareSurface small;
  SoftwareSurface original;
};

// std::string gen_filename(uint32_t num)
// {
//   std::ostringstream str;
//   str << "/tmp/tiledb/" << std::hex << num << ".png";
//   return str.str();
// }

int diff(const Pixel& lhs, const Pixel& rhs)
{
  int a = abs(static_cast<int>(lhs.tl) - static_cast<int>(rhs.tl));
  int b = abs(static_cast<int>(lhs.tr) - static_cast<int>(rhs.tr));
  int c = abs(static_cast<int>(lhs.bl) - static_cast<int>(rhs.bl));
  int d = abs(static_cast<int>(lhs.br) - static_cast<int>(rhs.br));

  return a*a + b*b + c*c + d*d;
}

Pixel get_pixels(SoftwareSurface const& surface)
{
  assert(surface.get_width()  == 4);
  assert(surface.get_height() == 4);

  return Pixel{
    surface.get_pixel({0, 0}).r8(),
    surface.get_pixel({1, 0}).r8(),
    surface.get_pixel({0, 1}).r8(),
    surface.get_pixel({1, 1}).r8()
  };
}

SoftwareSurface find_closest_tile(std::vector<TileDBEntry>& tiledb,
                                  SoftwareSurface const& input)
{
  Pixel input_pixel = get_pixels(input);

  SoftwareSurface best_match;
  int min_dist = std::numeric_limits<int>::max();

  for(const auto& entry : tiledb)
  {
    int dist = diff(entry.pixels, input_pixel);
    if (dist <= min_dist)
    {
      best_match = entry.original;
      min_dist = dist;
    }
  }

  return best_match;
}

} // namespace

int main(int argc, char* argv[])
{
  SoftwareSurfaceFactory surface_factory;

  std::vector<TileDBEntry> tiledb;

  // build tiledb
  for(int i = 1; i < argc; ++i)
  {
    std::cout << "Processing: " << argv[i] << std::endl;

    SoftwareSurface surface = surface_factory.from_file(argv[i]);
    Size tile_size(8, 8);

    for(int y = 0; y < surface.get_height() - tile_size.height(); y += tile_size.height())
    {
      std::cout << y << std::endl;
      for(int x = 0; x < surface.get_width() - tile_size.width(); x += tile_size.width())
      {
        SoftwareSurface src_tile = surf::crop(surface, Rect(Vector2i(x, y), tile_size));
        SoftwareSurface small = surf::halve(src_tile);
        tiledb.push_back(TileDBEntry{get_pixels(small), small, src_tile});
      }
    }

    //std::cout << gen_filename(0xdeadbeaf) << std::endl;
  }

  std::cout << "scaling images" << std::endl;
  // use tiledb to scale the images
  for(int i = argc-1; i < argc; ++i)
  {
    SoftwareSurface surface = surface_factory.from_file(argv[i]);
    SoftwareSurface out_surface = SoftwareSurface::create(surf::PixelFormat::RGB8, surface.get_size() * 2);
    Size tile_size(4, 4);

    for(int y = 0; y < surface.get_height() - tile_size.height(); y += tile_size.height())
    {
      std::cout << y << std::endl;
      for(int x = 0; x < surface.get_width() - tile_size.width(); x += tile_size.width())
      {
        SoftwareSurface src_tile = surf::crop(surface, Rect(Vector2i(x, y), tile_size));
        SoftwareSurface closest_tile = find_closest_tile(tiledb, src_tile);

        surf::blit(closest_tile, out_surface, Vector2i(x*2, y*2));
      }
    }

    png::save(out_surface, "/tmp/scaledup.png");
  }

  return 0;
}

/* EOF */
