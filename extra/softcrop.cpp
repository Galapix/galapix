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
#include <assert.h>

#include <surf/software_surface.hpp>
#include <surf/software_surface_factory.hpp>
#include <surf/color.hpp>

#include "plugins/png.hpp"
#include "plugins/jpeg.hpp"
#include "util/url.hpp"

using namespace surf;

int main(int argc, char* argv[])
{
  assert(argc != 3);
  SoftwareSurfaceFactory factory;

  //int left = atoi(argv[1]);
  std::string in_filename  = argv[2];
  std::string out_filename = argv[3];

  std::cout << "Loading: " << in_filename << " - writing to " << out_filename << std::endl;

  SoftwareSurface in = factory.from_file(in_filename);
  SoftwareSurface out = in;

  //png::save(out, out_filename);
  jpeg::save(out, out_filename, 85);

  return 0;
}

/* EOF */
