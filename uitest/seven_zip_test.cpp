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
#include <uitest/uitest.hpp>

#include "archive/seven_zip.hpp"
#include "archive/util.hpp"

UITEST(SevenZip, list, "FILENAME")
{
  const std::vector<std::string>& files = SevenZip::get_filenames(args[0]);
  for(std::vector<std::string>::const_iterator i = files.begin(); i != files.end(); ++i)
  {
    std::cout << "File: '" << *i << "'" << std::endl;
  }
}

UITEST(SevenZip, extract, "ARCHIVE FILENAME")
{
  auto data = SevenZip::get_file(args[0], args[1]);
  write_file("/tmp/out.file", data);
  std::cout << "Writting /tmp/out.file" << std::endl;
}

UITEST(SevenZip, fullextract, "ZIPFILE TARGETDIR")
{
  SevenZip::extract(args[0], args[1]);
}

/* EOF */
