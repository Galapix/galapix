/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2010 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_MATH_MATRIX2_HPP
#define HEADER_GALAPIX_MATH_MATRIX2_HPP

#include <iosfwd>
#include <math.h>
#include <assert.h>

#include "math/vector2f.hpp"

class Matrix2
{
private:
  /** matrix data in column-major ordering */
  float m_m[4];

public:
  static inline Matrix2 identity() 
  {
    return Matrix2(1.0f, 0.0f,
                   0.0f, 1.0f);
  }

  static inline Matrix2 scale(float x, float y)
  {
    return Matrix2(x, 0,
                   0, y);
  }

  static inline Matrix2 rotation(float rad)
  {
    const float c = cosf(rad);
    const float s = sinf(rad);

    return Matrix2(c, -s,
                   s,  c);
  }

public:
  inline Matrix2(float m[4])
  {
    for(int i = 0; i < 4; ++i)
      m_m[i] = m[i];
  }

  inline Matrix2(float m00, float m01,
                 float m10, float m11)
  {
    m_m[0] = m00;
    m_m[1] = m10;
    m_m[2] = m01;
    m_m[3] = m11;
  }
  
  inline float& operator()(int row, int col)
  {
    return m_m[2*col + row];
  }

  inline const float& operator()(int row, int col) const
  {
    return m_m[2*col + row];
  }

  inline Matrix2 operator*(const Matrix2& rhs) const
  {
    const Matrix2& lhs = *this;

    const float& a = lhs(0,0) * rhs(0,0) + lhs(0,1) * rhs(1,0);
    const float& b = lhs(0,0) * rhs(0,1) + lhs(0,1) * rhs(1,1);
    const float& c = lhs(1,0) * rhs(0,0) + lhs(1,1) * rhs(1,0);
    const float& d = lhs(1,0) * rhs(0,1) + lhs(1,1) * rhs(1,1);
    
    return Matrix2(a, b, 
                   c, d);
  }

  inline Vector2f operator*(const Vector2f& v) const
  {
    const Matrix2& m = *this;
    return Vector2f(v.x * m(0,0) + v.y * m(0,1),
                    v.x * m(1,0) + v.y * m(1,1));
  }

  inline float determinant() const
  {
    const Matrix2& m = *this;
    return m(0,0) * m(1,1) - m(0,1) * m(1,0);
  }

  inline Matrix2 inverse() const
  {
    const Matrix2& m = *this;
    float det = determinant();
    assert(det != 0.0f);
    return Matrix2(m(1,1) / det, -m(0,1) / det,
                   -m(1,0)/ det, m(0,0) / det);
  }

  inline Matrix2 transpose() const
  {
    const Matrix2& m = *this;
    return Matrix2(m(0,0), m(1,0),
                   m(0,1), m(1,1));
  }

  inline float* get() {
    return m_m;
  }

  inline const float* get() const {
    return m_m;
  }
};

std::ostream& operator<<(std::ostream& s, const Matrix2& m);

#endif

/* EOF */
