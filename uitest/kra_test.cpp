/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmx.de>
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
#include <stdlib.h>

#include "plugins/kra.hpp"
#include "plugins/png.hpp"

int main(int argc, char** argv)
{
  if (argc != 3)
  {
    std::cout << "Usage: " << argv[0] << " INFILE OUTFILE" << std::endl;
    return EXIT_FAILURE;
  }
  else
  {
    std::string infile  = argv[1];
    std::string outfile = argv[2];

    std::cout << "Trying to load  " << infile << std::endl;
    SoftwareSurfacePtr surface = KRA::load_from_file(infile);

    std::cout << "Trying to save  " << outfile << std::endl;
    PNG::save(surface, outfile);

    return EXIT_SUCCESS;
  }
}

/* EOF */
