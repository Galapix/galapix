/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#include "math/size.hpp"
#include "plugins/jpeg.hpp"

UITEST(JPEG, get_size, "FILE...")
{
  for(const auto& arg : args)
  {
    try
    {
      Size size = JPEG::get_size(arg);
      std::cout << "JPEG::get_size(\"" << arg << "\", " << size << ")" << std::endl;
    }
    catch(std::exception& err)
    {
      std::cout << err.what() << std::endl;
    }
  }
}

/* EOF */
