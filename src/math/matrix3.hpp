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

#ifndef HEADER_GALAPIX_MATH_MATRIX3_HPP
#define HEADER_GALAPIX_MATH_MATRIX3_HPP

#include <iosfwd>
#include <math.h>
#include <assert.h>

#include "math/math.hpp"
#include "math/vector2f.hpp"

class Matrix3
{
private:
  float m_m[9];
  
public:
  static inline Matrix3 identity() 
  {
    return Matrix3(1.0f, 0.0f, 0.0f,
                   0.0f, 1.0f, 0.0f,
                   0.0f, 0.0f, 1.0f);
  }

  static inline Matrix3 scale(float x, float y, float z = 1.0f)
  {
    return Matrix3(x, 0, 0,
                   0, y, 0,
                   0, 0, z);
  }

  static inline Matrix3 rotation(float rad, float x, float y, float z)
  {
    // normalize the input vector
    const float f = Math::sqrt(x*x + y*y + z*z);

    if (f != 0.0f)
    {
      x /= f;
      y /= f;
      z /= f;
    }

    const float c = cosf(rad);
    const float s = sinf(rad);

    return Matrix3(x*x * (1-c) + c,   x*y * (1-c) - z*s, x*z * (1-c) + y*s,
                   y*x * (1-c) + z*s, y*y * (1-c) + c,   y*z * (1-c) - x*s,
                   x*z * (1-c) - y*s, y*z * (1-c) + x*s, z*z * (1-c) + c);
  }

  static inline Matrix3 rotation_x(float rad)
  {
    const float c = cosf(rad);
    const float s = sinf(rad);

    return Matrix3(1, 0, 0,
                   0, c, -s,
                   0, s, c);
  }

  static inline Matrix3 rotation_y(float rad)
  {
    const float c = cosf(rad);
    const float s = sinf(rad);

    return Matrix3(c, 0, s,
                   0,  1, 0,
                   -s,  0, c);
  }

  static inline Matrix3 rotation_z(float rad)
  {
    const float c = cosf(rad);
    const float s = sinf(rad);

    return Matrix3(c,-s, 0,
                   s, c, 0,
                   0,  0, 1);
  }

  static inline Matrix3 translate(float tx, float ty)
  {
    return Matrix3(1, 0, tx,
                   0, 1, ty,
                   0, 0, 1);
  }

  Matrix3(float m00, float m01, float m02,
          float m10, float m11, float m12,
          float m20, float m21, float m22)
  {
    m_m[0] = m00;
    m_m[1] = m10;
    m_m[2] = m20;

    m_m[3] = m01;
    m_m[4] = m11;
    m_m[5] = m21;

    m_m[6] = m02;
    m_m[7] = m12;
    m_m[8] = m22;
  }
  
  inline float& operator()(int row, int col)
  {
    return m_m[3*col + row];
  }

  inline const float& operator()(int row, int col) const
  {
    return m_m[3*col + row];
  }

  inline Matrix3 operator*(const Matrix3& rhs) const
  {
    const Matrix3& lhs = *this;
   
    const float& a = lhs(0,0) * rhs(0,0) + lhs(0,1) * rhs(1,0) + lhs(0,2) * rhs(2,0);
    const float& b = lhs(0,0) * rhs(0,1) + lhs(0,1) * rhs(1,1) + lhs(0,2) * rhs(2,1);
    const float& c = lhs(0,0) * rhs(0,2) + lhs(0,1) * rhs(1,2) + lhs(0,2) * rhs(2,2);

    const float& d = lhs(1,0) * rhs(0,0) + lhs(1,1) * rhs(1,0) + lhs(1,2) * rhs(2,0);
    const float& e = lhs(1,0) * rhs(0,1) + lhs(1,1) * rhs(1,1) + lhs(1,2) * rhs(2,1);
    const float& f = lhs(1,0) * rhs(0,2) + lhs(1,1) * rhs(1,2) + lhs(1,2) * rhs(2,2);

    const float& g = lhs(2,0) * rhs(0,0) + lhs(2,1) * rhs(1,0) + lhs(2,2) * rhs(2,0);
    const float& h = lhs(2,0) * rhs(0,1) + lhs(2,1) * rhs(1,1) + lhs(2,2) * rhs(2,1);
    const float& i = lhs(2,0) * rhs(0,2) + lhs(2,1) * rhs(1,2) + lhs(2,2) * rhs(2,2);
   
    return Matrix3(a, b, c,
                   d, e, f,
                   g, h, i);
  }

  inline Vector2f operator*(const Vector2f& v) const
  {
    const Matrix3& m = *this;
    return Vector2f(v.x * m(0,0) + v.y * m(0,1) + m(0,2),
                    v.x * m(1,0) + v.y * m(1,1) + m(1,2));
  }

  inline Vector2f get_translate() const
  {
    const Matrix3& m = *this;
    return Vector2f(m(0, 2), m(1, 2));
  }

  inline float determinant() const
  {
    const Matrix3& m = *this;
    return
      m(0,0) * m(1,1) * m(2,2) +
      m(0,1) * m(1,2) * m(2,0) +
      m(0,2) * m(1,0) * m(2,1) +

      - m(0,0) * m(1,2) * m(2,1)
      - m(0,1) * m(1,0) * m(2,2)
      - m(0,2) * m(1,1) * m(2,0);
  }

  inline Matrix3 inverse() const
  {
    const Matrix3& m = *this;
    float det = determinant();
    assert(det != 0.0f);
    
    const float& a = m(1,1) * m(2,2) - m(1,2) * m(2,1);
    const float& b = m(0,2) * m(2,1) - m(0,1) * m(2,2);
    const float& c = m(0,1) * m(1,2) - m(0,2) * m(1,1);

    const float& d = m(1,2) * m(2,0) - m(1,0) * m(2,2);
    const float& e = m(0,0) * m(2,2) - m(0,2) * m(2,0);
    const float& f = m(0,2) * m(1,0) - m(0,0) * m(1,2);

    const float& g = m(1,0) * m(2,1) - m(1,1) * m(2,0);
    const float& h = m(0,1) * m(2,0) - m(0,0) * m(2,1);
    const float& i = m(0,0) * m(1,1) - m(0,1) * m(1,0);

    return Matrix3(a / det, b / det, c / det,
                   d / det, e / det, f / det,
                   g / det, h / det, i / det);
  }

  inline Matrix3 transpose() const
  {
    const Matrix3& m = *this;
    return Matrix3(m(0,0), m(1,0), m(2,0),
                   m(0,1), m(1,1), m(2,1),
                   m(0,2), m(1,2), m(2,2));
  }
};

std::ostream& operator<<(std::ostream& s, const Matrix3& m);

#endif

/* EOF */
