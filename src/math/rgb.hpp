/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#ifndef HEADER_MATH_RGB_HPP
#define HEADER_MATH_RGB_HPP

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

  RGB(uint8_t r, uint8_t g, uint8_t b)
    : r(r), g(g), b(b)
  {}

  RGB(uint32_t c)
    : r((c>> 0) & 0xFF), 
      g((c>> 8) & 0xFF),
      b((c>>16) & 0xFF)
  {}

  uint32_t get_uint32() const {
    return ((r << 0) | (g << 8) | (b << 16));
  }
};

#endif

/* EOF */
