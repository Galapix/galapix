// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_MATH_MATH_HPP
#define HEADER_GALAPIX_MATH_MATH_HPP

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string>

namespace Math {

/** Calculates 2^n */
inline int pow2(int n)
{
  return (1 << n);
}

/** Does a/b and rounds the result up */
inline int ceil_div(int a, int b)
{
  return (a + b - 1) / b;
}

} // namespace Math

#endif

/* EOF */
