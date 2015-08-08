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

#include "archive/rar.hpp"
#include "archive/rar_header.hpp"

#include <iostream>
#include <uitest/uitest.hpp>

UITEST_S(Rar, info, "ARCHIVE")
{
  auto header = RarHeader::from_file(arg);
  std::cout << arg << ":"
            << " flags = " << header.get_flags()
            << " is_encrypted = " << header.is_encrypted()
            << " has_recovery = " << header.has_recovery()
            << " is_multi_volume = " << header.is_multi_volume()
            << " is_first_volume = " << header.is_first_volume()
            << " has_new_volume_naming = " << header.has_new_volume_naming()
            << std::endl;
}

UITEST(Rar, list, "ARCHIVE")
{
  const std::vector<std::string>& files = Rar::get_filenames(args[0]);
  for(std::vector<std::string>::const_iterator i = files.begin(); i != files.end(); ++i)
  {
    std::cout << "File: '" << *i << "'" << std::endl;
  }
}

UITEST(Rar, extract, "ARCHIVE FILENAME")
{
  Blob blob = Rar::get_file(args[0], args[1]);
  blob.write_to_file("/tmp/out.file");
  std::cout << "Writting /tmp/out.file" << std::endl;
}

/* EOF */
