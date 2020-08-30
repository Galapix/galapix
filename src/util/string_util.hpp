//  SuperTux
//  Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_UTIL_STRING_UTIL_HPP
#define HEADER_GALAPIX_UTIL_STRING_UTIL_HPP

#include <string>
#include <sstream>

class StringUtil
{
public:
  static bool has_suffix(const std::string& data, const std::string& suffix);
  static bool has_prefix(const std::string& lhs, const std::string& rhs);

  /** Compare two strings according to their numeric value, similar to
      what 'sort -n' does. */
  static bool numeric_less(const std::string& lhs, const std::string& rhs);
};

std::vector<std::string> string_tokenize(std::string_view text, char delimiter);

template<class T>
bool from_string(const std::string& s, T& t)
{
  std::istringstream str(s);
  T tmp;
  str >> tmp;
  if (str.fail())
  {
    return false;
  }
  else
  {
    t = tmp;
    return true;
  }
  return false;
}

#endif

/* EOF */
