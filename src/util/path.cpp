/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2018 Ingo Ruhnke <grumbel@gmail.com>
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

#include "util/path.hpp"

#include <random>

namespace galapix {

std::filesystem::path unique_path()
{
  const char charset[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";

  std::random_device rnd;
  std::uniform_int_distribution<int> distributed(0, std::size(charset) - 1);

  std::string str(16, ' ');
  for(size_t i = 0; i < str.size(); ++i)
  {
    str[i] = charset[distributed(rnd)];
  }

  return str;
}

} // namespace galapix

/* EOF */
