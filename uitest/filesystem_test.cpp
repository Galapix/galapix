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
#include <iomanip>
#include <uitest/uitest.hpp>

#include "util/filesystem.hpp"

UITEST(Filesystem, test, "FILE...")
{
  for(const auto& arg : args)
  {
    std::string extension = Filesystem::get_extension(arg);
    std::cout << "'" << extension << "' - " << arg << std::endl;
  }
}

UITEST(Filesystem, get_magic, "FILE...",
       "Filesystem::get_magic()")
{
  for(const auto& arg : args)
  {
    std::string magic = Filesystem::get_magic(arg);
    std::cout << arg << ": { ";
    for(auto it = magic.begin(); it != magic.end(); ++it)
    {
      std::cout << std::setfill('0') << std::hex << std::setw(2) << static_cast<int>(*it) << " ";
    }
    std::cout << "}" << std::endl;
  }
}

/* EOF */
