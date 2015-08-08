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

#include "math/size.hpp"
#include "math/vector2i.hpp"
#include "math/rect.hpp"
#include "util/software_surface_factory.hpp"
#include "math/rgb.hpp"
#include "plugins/png.hpp"

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
  SoftwareSurfacePtr small;
  SoftwareSurfacePtr original;
};

std::string gen_filename(uint32_t num)
{
  std::ostringstream str;
  str << "/tmp/tiledb/" << std::hex << num << ".png";
  return str.str();
}

int diff(const Pixel& lhs, const Pixel& rhs)
{
  int a = abs(static_cast<int>(lhs.tl) - static_cast<int>(rhs.tl));
  int b = abs(static_cast<int>(lhs.tr) - static_cast<int>(rhs.tr));
  int c = abs(static_cast<int>(lhs.bl) - static_cast<int>(rhs.bl));
  int d = abs(static_cast<int>(lhs.br) - static_cast<int>(rhs.br));

  return a*a + b*b + c*c + d*d;
}

Pixel get_pixels(SoftwareSurfacePtr surface)
{
  assert(surface->get_width()  == 4);
  assert(surface->get_height() == 4);

  RGB tl;
  RGB tr;
  RGB bl;
  RGB br;

  surface->get_pixel(0, 0, tl);
  surface->get_pixel(1, 0, tr);
  surface->get_pixel(0, 1, bl);
  surface->get_pixel(1, 1, br);

  return Pixel{tl.r, tr.r, bl.r, br.r};
}

SoftwareSurfacePtr find_closest_tile(std::vector<TileDBEntry>& tiledb,
                                     SoftwareSurfacePtr input)
{
  Pixel input_pixel = get_pixels(input);

  SoftwareSurfacePtr best_match;
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

int main(int argc, char* argv[])
{
  SoftwareSurfaceFactory surface_factory;

  std::vector<TileDBEntry> tiledb;

  // build tiledb
  for(int i = 1; i < argc; ++i)
  {
    std::cout << "Processing: " << argv[i] << std::endl;

    SoftwareSurfacePtr surface = SoftwareSurfaceFactory::current().from_file(argv[i]);
    Size tile_size(8, 8);

    for(int y = 0; y < surface->get_height() - tile_size.height; y += tile_size.height)
    {
      std::cout << y << std::endl;
      for(int x = 0; x < surface->get_width() - tile_size.width; x += tile_size.width)
      {
        SoftwareSurfacePtr src_tile = surface->crop(Rect(Vector2i(x, y), tile_size));
        SoftwareSurfacePtr small = src_tile->halve();
        tiledb.push_back(TileDBEntry{get_pixels(small), small, src_tile});
      }
    }

    //std::cout << gen_filename(0xdeadbeaf) << std::endl;
  }

  std::cout << "scaling images" << std::endl;
  // use tiledb to scale the images
  for(int i = argc-1; i < argc; ++i)
  {
    SoftwareSurfacePtr surface = SoftwareSurfaceFactory::current().from_file(argv[i]);
    SoftwareSurfacePtr out_surface = SoftwareSurface::create(SoftwareSurface::RGB_FORMAT,
                                                             Size(surface->get_size() * 2));
    Size tile_size(4, 4);

    for(int y = 0; y < surface->get_height() - tile_size.height; y += tile_size.height)
    {
      std::cout << y << std::endl;
      for(int x = 0; x < surface->get_width() - tile_size.width; x += tile_size.width)
      {
        SoftwareSurfacePtr src_tile = surface->crop(Rect(Vector2i(x, y), tile_size));

        SoftwareSurfacePtr closest_tile = find_closest_tile(tiledb, src_tile);

        closest_tile->blit(*out_surface, Vector2i(x*2, y*2));
      }
    }

    PNG::save(out_surface, "/tmp/scaledup.png");
  }

  return 0;
}

/* EOF */
