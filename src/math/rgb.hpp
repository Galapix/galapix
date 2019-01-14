/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_MATH_RGB_HPP
#define HEADER_GALAPIX_MATH_RGB_HPP

#include <stdint.h>

class RGB
{
public:
  uint8_t r;
  uint8_t g;
  uint8_t b;

  RGB()
    : r(0), g(0), b(0)
  {}

  RGB(uint8_t r_, uint8_t g_, uint8_t b_)
    : r(r_), g(g_), b(b_)
  {}

  RGB(uint32_t c)
    : r(static_cast<uint8_t>((c>> 0) & 0xFF)),
      g(static_cast<uint8_t>((c>> 8) & 0xFF)),
      b(static_cast<uint8_t>((c>>16) & 0xFF))
  {}

  uint32_t get_uint32() const {
    return ((static_cast<uint32_t>(r) << 0) |
            (static_cast<uint32_t>(g) << 8) |
            (static_cast<uint32_t>(b) << 16));
  }
};

#endif

/* EOF */
