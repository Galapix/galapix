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
#include <uitest/uitest.hpp>

#include "archive/archive_manager.hpp"

UITEST(ArchiveManager, is_archive, "ARCHIVENAME...",
       "Check if the given filename is an archive or not")
{
  ArchiveManager archiver;

  for(const auto& arg : rest)
  {
    std::cout << "filename: " << arg << std::endl;
    std::cout << "is_archive(): " << archiver.is_archive(arg) << std::endl;
    std::cout << std::endl;
  }
}

UITEST(ArchiveManager, list, "ARCHIVENAME...",
       "List the content of the given archive")
{
  ArchiveManager archiver;

  for(const auto& arg : rest)
  {
    std::cout << "filename: " << arg << std::endl;

    try
    {
      auto lst = archiver.get_filenames(arg);

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
}

UITEST(ArchiveManager, extract, "ARCHIVENAME FILENAME OUTFILE",
       "Extract a single file out of an archive")
{
  std::string archive  = args[0];
  std::string filename = args[1];
  std::string outfile  = args[2];

  ArchiveManager archiver;
  Blob blob = archiver.get_file(archive, filename);
  blob.write_to_file(outfile);
}

/* EOF */
