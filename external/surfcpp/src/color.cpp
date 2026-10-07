// surf - Software surface library
// Copyright (C) 2008-2020 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <format>

#include "color.hpp"
#include "palette.hpp"

namespace surf {

namespace {

uint8_t x2i(char c)
{
  switch (c) {
    case '0': return 0;
    case '1': return 1;
    case '2': return 2;
    case '3': return 3;
    case '4': return 4;
    case '5': return 5;
    case '6': return 6;
    case '7': return 7;
    case '8': return 8;
    case '9': return 9;
    case 'A': case 'a': return 0xa;
    case 'B': case 'b': return 0xb;
    case 'C': case 'c': return 0xc;
    case 'D': case 'd': return 0xd;
    case 'E': case 'e': return 0xe;
    case 'F': case 'f': return 0xf;
    default:
      throw std::invalid_argument(std::format("not a valid hex character: '{}'", c));
  }
}

} // namespace

Color
Color::from_string(std::string_view text) {
  if (text.empty()) { return {}; }

  float r = 1.0f;
  float g = 1.0f;
  float b = 1.0f;
  float a = 1.0f;

  std::string str(text);

  if (text[0] == '#') {
    if (text.size() == 4) {
      return Color::from_rgb888(x2i(text[1]) * 17,
                                x2i(text[2]) * 17,
                                x2i(text[3]) * 17);
    } else if (text.size() == 7) {
      return Color::from_rgb888(static_cast<uint8_t>((x2i(text[1]) << 4) | x2i(text[2])),
                                static_cast<uint8_t>((x2i(text[3]) << 4) | x2i(text[4])),
                                static_cast<uint8_t>((x2i(text[5]) << 4) | x2i(text[6])));
    } else {
      throw std::invalid_argument(std::format("invalid color string: {}", text));
    }
  } else if (sscanf(str.c_str(), " %f, %f, %f, %f ", &r, &g, &b, &a) == 4 ||
             sscanf(str.c_str(), " %f, %f, %f ", &r, &g, &b) == 3) {
    return Color(r, g, b, a);
  } else {
    return palette::lookup_colorname(text);
  }
}

} // namespace surf

/* EOF */
