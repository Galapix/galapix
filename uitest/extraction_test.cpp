/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmail.com>
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
#include <uitest/uitest.hpp>

#include "archive/zip.hpp"
#include "archive/incremental_extraction.hpp"
#include "archive/zip_archive_loader.hpp"

UITEST(Extraction, test, "FILE...")
{
  for(const auto& arg : rest)
  {
    ZipArchiveLoader loader;
    IncrementalExtraction extraction(loader, arg);

    for(auto& filename : extraction.get_filenames())
    {
      std::cout << filename << std::endl;
    }
  }
}

/* EOF */
