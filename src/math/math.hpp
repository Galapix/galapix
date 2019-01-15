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

/** A collection of small math helper functions, some of them might be
    equal in functionality to standard STL functions, but provided
    here for portability and broken STL implementations

    @brief A collection of mathematical helper functions */
namespace Math {

constexpr float pi   = 3.14159265358979323846f;/* pi */
constexpr float pi_2 = 1.57079632679489661923f;/* pi/2 */
constexpr float tau  = 3.14159265358979323846f * 2.0f; /* 2*pi */

// Win32 defines these are defines already, so we have to undef them
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

template<class T>
T min(const T& a, const T& b)
{
  if (a < b) {
    return a;
  } else {
    return b;
  }
}

template<class T>
T max(const T& a, const T& b)
{
  if (a > b) {
    return a;
  } else {
    return b;
  }
}

template<class T>
T clamp(const T& low, const T& v, const T& high)
{
  assert(low <= high);
  return max((low), min((v), (high)));
}

inline
float frand()
{
  return static_cast<float>(rand()) / (static_cast<float>(RAND_MAX) + 1.0f);
}

inline
bool rand_bool()
{
  return rand()%2 == 0;
}

inline int round(float f)
{
  return static_cast<int>(::round(f));
}

float abs(float v);
float sin(float a);
float cos(float a);
float sqrt(float a);
float mod(float x, float y);
float floor(float x);
float atan2(float x, float y);

/** Write out the raw bits of a float as hex */
std::string float2string(float value);

/** Restore the raw bits of a float from a string */
float string2float(const std::string& str);

inline int round_to_power_of_two(int n)
{
  n = n - 1;

  n = n | (n >> 1);
  n = n | (n >> 2);
  n = n | (n >> 4);
  n = n | (n >> 8);
  n = n | (n >> 16);

  n = n + 1;

  return n;
}

/** Calculates 2^n */
inline int pow2(int n)
{
  return (1 << n);
}

inline bool is_power_of_two(int n)
{
  return (n > 0) && ((n & (n - 1)) == 0);
}

/** Does a/b and rounds the result up */
inline int ceil_div(int a, int b)
{
  return (a + b - 1) / b;
}

inline float rad2deg(float v)
{
  return v / tau * 360.0f;
}

inline float deg2rad(float v)
{
  return v / 360.0f * tau;
}

} // namespace Math

#endif

/* EOF */
