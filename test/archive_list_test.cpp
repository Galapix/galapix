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

#include <iostream>

#include "util/archive_manager.hpp"

int main(int argc, char** argv)
{
  if (argc == 1)
  {
    std::cout << "Usage: " << argv[0] << " ARCHIVENAME..." << std::endl;
    return 1;
  }
  else
  {
    ArchiveManager archiver;

    for(int i = 1; i < argc; ++i)
    {
      std::cout << "filename: " << argv[i] << std::endl;

      try
      {
        auto lst = archiver.get_filenames(argv[i]);
  
        for(const auto& filename: lst)
        {
          std::cout << "  " << filename << std::endl;
        }
      }
      catch(const std::exception& err)
      {
        std::cout << "error: " << err.what() << std::endl;
      }

      std::cout << std::endl;
    }
    return 0;
  }
}

/* EOF */
