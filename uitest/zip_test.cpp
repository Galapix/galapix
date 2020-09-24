/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmail.com>
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

#include "archive/util.hpp"
#include "archive/zip.hpp"

UITEST(ZipTest, list, "ZIPFILE",
       "List files in a .zip")
{
  const std::vector<std::string>& files = Zip::get_filenames(args[0]);
  for(std::vector<std::string>::const_iterator i = files.begin(); i != files.end(); ++i)
  {
    std::cout << "File: '" << *i << "'" << std::endl;
  }
}

UITEST(ZipTest, extract, "ZIPFILE FILETOEXTRACT",
       "Extract a file from a .zip")
{
  auto blob = Zip::get_file(args[0], args[1]);
  write_file("/tmp/out.file", blob);
  std::cout << "Writting /tmp/out.file" << std::endl;
}

/* EOF */
