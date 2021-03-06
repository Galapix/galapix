/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_MATH_RGBF_HPP
#define HEADER_GALAPIX_MATH_RGBF_HPP

#include "math/rgba.hpp"
#include "math/rgb.hpp"

class RGBAf
{
public:
  float r;
  float g; 
  float b;
  float a;

  RGBAf() :
    r(0.0f), g(0.0f), b(0.0f), a(0.0f)
  {}

  RGBAf(float r_,
        float g_,
        float b_,
        float a_) :
    r(r_), g(g_), b(b_), a(a_)
  {}

  static RGBAf from_rgb(const RGB& rgb)
  {
    return RGBAf(static_cast<float>(rgb.r)/255.0f,
                 static_cast<float>(rgb.g)/255.0f, 
                 static_cast<float>(rgb.b)/255.0f, 
                 1.0f);
  }

  static RGBAf from_rgba(const RGBA& rgba)
  {
    return RGBAf(static_cast<float>(rgba.r)/255.0f,
                 static_cast<float>(rgba.g)/255.0f,
                 static_cast<float>(rgba.b)/255.0f,
                 static_cast<float>(rgba.a)/255.0f);
  }
};

#endif

/* EOF */
