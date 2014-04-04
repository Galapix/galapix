/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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

#include "archive/zip.hpp"
#include "archive/incremental_extraction.hpp"
#include "archive/zip_archive_loader.hpp"

int main(int argc, char** argv)
{
  if (argc == 2)
  {
    ZipArchiveLoader loader;
    IncrementalExtraction extraction(loader, argv[1]);
    
    for(auto& filename : extraction.get_filenames())
    {
      std::cout << filename << std::endl;
    }
    return 0;
  }
  else if (argc == 3)
  {
    ZipArchiveLoader loader;
    IncrementalExtraction extraction(loader, argv[1]);
    std::string path = extraction.get_file_as_path(argv[2]);
    std::cout << path << std::endl;
    return 0;
  }
  else
  {
    std::cout << "Usage: " << argv[0] << " ARCHIVE [FILENAME]" << std::endl;
    return 1;
  }
}

/* EOF */
