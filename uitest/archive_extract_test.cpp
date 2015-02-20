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

#include "archive/archive_manager.hpp"

int main(int argc, char** argv)
{
  if (argc != 4)
  {
    std::cout << "Usage: " << argv[0] << " ARCHIVENAME FILENAME OUTFILE" << std::endl;
    std::cout << "Extract a single file out of an archive" << std::endl;
    return 1;
  }
  else
  {
    ArchiveManager archiver;

    std::string archive  = argv[1];
    std::string filename = argv[2];
    std::string outfile  = argv[3];

    BlobPtr blob = archiver.get_file(archive, filename);
    blob->write_to_file(outfile);

    return 0;
  }
}

/* EOF */
