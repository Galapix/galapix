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

#ifndef HEADER_GALAPIX_MATH_MATRIX4_HPP
#define HEADER_GALAPIX_MATH_MATRIX4_HPP

#include <iosfwd>
#include <math.h>
#include <assert.h>

#include "math/math.hpp"
#include "math/vector3f.hpp"

class Matrix4
{
private:
  /** matrix data in column-major ordering */
  float m_m[16];

public:
  Matrix4() :
    m_m()
  {}

  Matrix4(float m[16])
  {
    for(int i = 0; i < 16; ++i)
      m_m[i] = m[i];
  }

  Matrix4(float m00, float m01, float m02, float m03,
          float m10, float m11, float m12, float m13,
          float m20, float m21, float m22, float m23,
          float m30, float m31, float m32, float m33)
  {
    m_m[0] = m00;
    m_m[1] = m10;
    m_m[2] = m20;
    m_m[3] = m30;

    m_m[4] = m01;
    m_m[5] = m11;
    m_m[6] = m21;
    m_m[7] = m31;

    m_m[8]  = m02;
    m_m[9]  = m12;
    m_m[10] = m22;
    m_m[11] = m32;

    m_m[12] = m03;
    m_m[13] = m13;
    m_m[14] = m23;
    m_m[15] = m33;
  }
  
  inline float& operator()(int row, int col)
  {
    return m_m[4*col + row];
  }

  inline const float& operator()(int row, int col) const
  {
    return m_m[4*col + row];
  }

  static inline Matrix4 identity()
  {
    return Matrix4(1, 0, 0, 0, 
                   0, 1, 0, 0, 
                   0, 0, 1, 0,
                   0, 0, 0, 1);
  }

  static inline Matrix4 scale(float x, float y, float z, float w = 1.0f)
  {
    return Matrix4(x, 0, 0, 0, 
                   0, y, 0, 0, 
                   0, 0, z, 0,
                   0, 0, 0, 1);
  }

  static inline Matrix4 rotation(float rad, float x, float y, float z)
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

    return Matrix4(x*x * (1-c) + c,   x*y * (1-c) - z*s, x*z * (1-c) + y*s, 0,
                   y*x * (1-c) + z*s, y*y * (1-c) + c,   y*z * (1-c) - x*s, 0,
                   x*z * (1-c) - y*s, y*z * (1-c) + x*s, z*z * (1-c) + c,   0,
                   0,                 0,                 0,                 1);
  }

  static inline Matrix4 rotation_x(float rad)
  {
    const float c = cosf(rad);
    const float s = sinf(rad);

    return Matrix4(1, 0, 0, 0,
                   0, c,-s, 0,
                   0, s, c, 0,
                   0, 0, 0, 1);
  }

  static inline Matrix4 rotation_y(float rad)
  {
    const float c = cosf(rad);
    const float s = sinf(rad);

    return Matrix4(c, 0, s, 0,
                   0, 1, 0, 0, 
                   -s,0, c, 0,
                   0, 0, 0, 1);
  }

  static inline Matrix4 rotation_z(float rad)
  {
    const float c = cosf(rad);
    const float s = sinf(rad);

    return Matrix4(c,-s, 0, 0, 
                   s, c, 0, 0,
                   0, 0, 1, 0,
                   0, 0, 0, 1);
  }

  static inline Matrix4 translate(float tx, float ty, float tz)
  {
    return Matrix4(1, 0, 0, tx,
                   0, 1, 0, ty,
                   0, 0, 1, tz,
                   0, 0, 0, 1);
  }

  inline Vector3f operator*(const Vector3f& v) const
  {
    const Matrix4& m = *this;

    Vector3f out(v.x * m(0,0) + v.y * m(0,1) + v.z * m(0,2) + m(0,3),
                 v.x * m(1,0) + v.y * m(1,1) + v.z * m(1,2) + m(1,3),
                 v.x * m(2,0) + v.y * m(2,1) + v.z * m(2,2) + m(2,3));

    float w = v.x * m(3,0) + v.y * m(3,1) + v.z * m(3,2) + m(3,3);
    
    if (w != 1.0f)
    {
      out.x /= w;
      out.y /= w;
      out.z /= w;
    }
    
    return out;
  }

  inline Matrix4 operator*(const Matrix4& rhs) const
  {
    const Matrix4& lhs = *this;
   
    return Matrix4(
      lhs(0,0) * rhs(0,0) + lhs(0,1) * rhs(1,0) + lhs(0,2) * rhs(2,0) + lhs(0,3) * rhs(3,0),
      lhs(0,0) * rhs(0,1) + lhs(0,1) * rhs(1,1) + lhs(0,2) * rhs(2,1) + lhs(0,3) * rhs(3,1),
      lhs(0,0) * rhs(0,2) + lhs(0,1) * rhs(1,2) + lhs(0,2) * rhs(2,2) + lhs(0,3) * rhs(3,2),
      lhs(0,0) * rhs(0,3) + lhs(0,1) * rhs(1,3) + lhs(0,2) * rhs(2,3) + lhs(0,3) * rhs(3,3),
      
      lhs(1,0) * rhs(0,0) + lhs(1,1) * rhs(1,0) + lhs(1,2) * rhs(2,0) + lhs(1,3) * rhs(3,0),
      lhs(1,0) * rhs(0,1) + lhs(1,1) * rhs(1,1) + lhs(1,2) * rhs(2,1) + lhs(1,3) * rhs(3,1),
      lhs(1,0) * rhs(0,2) + lhs(1,1) * rhs(1,2) + lhs(1,2) * rhs(2,2) + lhs(1,3) * rhs(3,2),
      lhs(1,0) * rhs(0,3) + lhs(1,1) * rhs(1,3) + lhs(1,2) * rhs(2,3) + lhs(1,3) * rhs(3,3),
      
      lhs(2,0) * rhs(0,0) + lhs(2,1) * rhs(1,0) + lhs(2,2) * rhs(2,0) + lhs(2,3) * rhs(3,0),
      lhs(2,0) * rhs(0,1) + lhs(2,1) * rhs(1,1) + lhs(2,2) * rhs(2,1) + lhs(2,3) * rhs(3,1),
      lhs(2,0) * rhs(0,2) + lhs(2,1) * rhs(1,2) + lhs(2,2) * rhs(2,2) + lhs(2,3) * rhs(3,2),
      lhs(2,0) * rhs(0,3) + lhs(2,1) * rhs(1,3) + lhs(2,2) * rhs(2,3) + lhs(2,3) * rhs(3,3),

      lhs(3,0) * rhs(0,0) + lhs(3,1) * rhs(1,0) + lhs(3,2) * rhs(2,0) + lhs(3,3) * rhs(3,0),
      lhs(3,0) * rhs(0,1) + lhs(3,1) * rhs(1,1) + lhs(3,2) * rhs(2,1) + lhs(3,3) * rhs(3,1),
      lhs(3,0) * rhs(0,2) + lhs(3,1) * rhs(1,2) + lhs(3,2) * rhs(2,2) + lhs(3,3) * rhs(3,2),
      lhs(3,0) * rhs(0,3) + lhs(3,1) * rhs(1,3) + lhs(3,2) * rhs(2,3) + lhs(3,3) * rhs(3,3));
  }

  inline Matrix4 transpose() const
  {
    const Matrix4& m = *this;
    return Matrix4(m(0,0), m(1,0), m(2,0), m(3,0),
                   m(0,1), m(1,1), m(2,1), m(3,1),
                   m(0,2), m(1,2), m(2,2), m(3,2),
                   m(0,3), m(1,3), m(2,3), m(3,3));
  }
  
  inline float determinant() const
  {
    const float* m = m_m;

    // code is based on Matrix4::inverse()
    float inv_0  =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
    float inv_4  = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
    float inv_8  =  m[4]*m[9]*m[15]  - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
    float inv_12 = -m[4]*m[9]*m[14]  + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];

    return m[0]*inv_0 + m[1]*inv_4 + m[2]*inv_8 + m[3]*inv_12;
  }

  inline Matrix4 inverse() const
  {
    /* Code by David Moore (See Mesa bug #6748)
       Licensed under http://oss.sgi.com/projects/FreeB/
       Modified to fit into the Matrix4 class */
    Matrix4 inv_m;
    float* inv = inv_m.m_m;
    const float* m = m_m;

    inv[0]  =  m[5]*m[10]*m[15] - m[5]*m[11]*m[14] - m[9]*m[6]*m[15] + m[9]*m[7]*m[14] + m[13]*m[6]*m[11] - m[13]*m[7]*m[10];
    inv[4]  = -m[4]*m[10]*m[15] + m[4]*m[11]*m[14] + m[8]*m[6]*m[15] - m[8]*m[7]*m[14] - m[12]*m[6]*m[11] + m[12]*m[7]*m[10];
    inv[8]  =  m[4]*m[9]*m[15]  - m[4]*m[11]*m[13] - m[8]*m[5]*m[15] + m[8]*m[7]*m[13] + m[12]*m[5]*m[11] - m[12]*m[7]*m[9];
    inv[12] = -m[4]*m[9]*m[14]  + m[4]*m[10]*m[13] + m[8]*m[5]*m[14] - m[8]*m[6]*m[13] - m[12]*m[5]*m[10] + m[12]*m[6]*m[9];
    inv[1]  = -m[1]*m[10]*m[15] + m[1]*m[11]*m[14] + m[9]*m[2]*m[15] - m[9]*m[3]*m[14] - m[13]*m[2]*m[11] + m[13]*m[3]*m[10];
    inv[5]  =  m[0]*m[10]*m[15] - m[0]*m[11]*m[14] - m[8]*m[2]*m[15] + m[8]*m[3]*m[14] + m[12]*m[2]*m[11] - m[12]*m[3]*m[10];
    inv[9]  = -m[0]*m[9]*m[15]  + m[0]*m[11]*m[13] + m[8]*m[1]*m[15] - m[8]*m[3]*m[13] - m[12]*m[1]*m[11] + m[12]*m[3]*m[9];
    inv[13] =  m[0]*m[9]*m[14]  - m[0]*m[10]*m[13] - m[8]*m[1]*m[14] + m[8]*m[2]*m[13] + m[12]*m[1]*m[10] - m[12]*m[2]*m[9];
    inv[2]  =  m[1]*m[6]*m[15]  - m[1]*m[7]*m[14]  - m[5]*m[2]*m[15] + m[5]*m[3]*m[14] + m[13]*m[2]*m[7]  - m[13]*m[3]*m[6];
    inv[6]  = -m[0]*m[6]*m[15]  + m[0]*m[7]*m[14]  + m[4]*m[2]*m[15] - m[4]*m[3]*m[14] - m[12]*m[2]*m[7]  + m[12]*m[3]*m[6];
    inv[10] =  m[0]*m[5]*m[15]  - m[0]*m[7]*m[13]  - m[4]*m[1]*m[15] + m[4]*m[3]*m[13] + m[12]*m[1]*m[7]  - m[12]*m[3]*m[5];
    inv[14] = -m[0]*m[5]*m[14]  + m[0]*m[6]*m[13]  + m[4]*m[1]*m[14] - m[4]*m[2]*m[13] - m[12]*m[1]*m[6]  + m[12]*m[2]*m[5];
    inv[3]  = -m[1]*m[6]*m[11]  + m[1]*m[7]*m[10]  + m[5]*m[2]*m[11] - m[5]*m[3]*m[10] - m[9]*m[2]*m[7]   + m[9]*m[3]*m[6];
    inv[7]  =  m[0]*m[6]*m[11]  - m[0]*m[7]*m[10]  - m[4]*m[2]*m[11] + m[4]*m[3]*m[10] + m[8]*m[2]*m[7]   - m[8]*m[3]*m[6];
    inv[11] = -m[0]*m[5]*m[11]  + m[0]*m[7]*m[9]   + m[4]*m[1]*m[11] - m[4]*m[3]*m[9]  - m[8]*m[1]*m[7]   + m[8]*m[3]*m[5];
    inv[15] =  m[0]*m[5]*m[10]  - m[0]*m[6]*m[9]   - m[4]*m[1]*m[10] + m[4]*m[2]*m[9]  + m[8]*m[1]*m[6]   - m[8]*m[2]*m[5];

    float det = m[0]*inv[0] + m[1]*inv[4] + m[2]*inv[8] + m[3]*inv[12];

    assert(det != 0);
    det = 1.0f / det;

    for (int i = 0; i < 16; ++i)
      inv[i] *= det;

    return inv_m;
  }

  inline float* get() {
    return m_m;
  }

  inline const float* get() const {
    return m_m;
  }
};

std::ostream& operator<<(std::ostream& s, const Matrix4& m);

#endif

/* EOF */
