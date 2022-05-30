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
#include <assert.h>

#include "util/sha1.hpp"

using namespace galapix;

UITEST_S(SHA1, test, "FILE", "")
{
  std::cout << galapix::SHA1::from_file(arg).str() << "  " << arg << std::endl;
}

UITEST_S(SHA1, parse_string, "TEXT", "")
{
  std::string const& in = arg;
  std::string out = galapix::SHA1::from_string(in).str();
  std::cout <<  in << "  " << out << std::endl;
  assert(in == out);
}

/* EOF */
