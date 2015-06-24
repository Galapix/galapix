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

#ifndef HEADER_GALAPIX_PLUGINS_DDS_RGB_HPP
#define HEADER_GALAPIX_PLUGINS_DDS_RGB_HPP

#include <stdint.h>

struct DDSRGB
{
public:
  typedef uint32_t dds_int_t;

  dds_int_t r;
  dds_int_t g;
  dds_int_t b;

  DDSRGB() : r(0), g(0), b(0) {}

  DDSRGB(unsigned short color) :
    r(static_cast<dds_int_t>((color & 0x1F) << 3)),
    g(static_cast<dds_int_t>(((color & 0x7E0) >> 5) << 2)),
    b(static_cast<dds_int_t>(((color & 0xF800) >> 11) << 3))
  {
  }

  DDSRGB& operator=(unsigned short color)
  {
    r = static_cast<dds_int_t>((color & 0x1F) << 3);
    g = static_cast<dds_int_t>(((color & 0x7E0) >> 5) << 2);
    b = static_cast<dds_int_t>(((color & 0xF800) >> 11) << 3);

    return *this;
  }
};

inline DDSRGB operator*(unsigned int s, const DDSRGB& a)
{
  DDSRGB result;
  result.r = static_cast<DDSRGB::dds_int_t>(a.r * s);
  result.g = static_cast<DDSRGB::dds_int_t>(a.g * s);
  result.b = static_cast<DDSRGB::dds_int_t>(a.b * s);
  return result;
}

inline DDSRGB operator*(const DDSRGB& a, unsigned int s)
{
  DDSRGB result;
  result.r = static_cast<DDSRGB::dds_int_t>(a.r * s);
  result.g = static_cast<DDSRGB::dds_int_t>(a.g * s);
  result.b = static_cast<DDSRGB::dds_int_t>(a.b * s);
  return result;
}

inline DDSRGB operator/(const DDSRGB& a, unsigned int s)
{
  DDSRGB result;
  result.r = static_cast<DDSRGB::dds_int_t>(a.r / s);
  result.g = static_cast<DDSRGB::dds_int_t>(a.g / s);
  result.b = static_cast<DDSRGB::dds_int_t>(a.b / s);
  return result;
}

inline DDSRGB operator+(const DDSRGB& a, const DDSRGB& b)
{
  DDSRGB result;
  result.r = static_cast<DDSRGB::dds_int_t>(a.r + b.r);
  result.g = static_cast<DDSRGB::dds_int_t>(a.g + b.g);
  result.b = static_cast<DDSRGB::dds_int_t>(a.b + b.b);
  return result;
}


#endif

/* EOF */
