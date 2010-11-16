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

#include <iostream>
#include <stdlib.h>

#include "math/math.hpp"
#include "math/matrix4.hpp"

int main(int argc, char* argv[])
{
  srand(time(NULL));

  Matrix4 s(0, 4, 8,  12,
            1, 5, 9,  13,
            2, 6, 10, 14,
            3, 7, 11, 15);
  
  Matrix4 m(Math::frand()*10.0f, Math::frand()*10.0f, Math::frand()*10.0f, Math::frand()*10.0f,
            Math::frand()*10.0f, Math::frand()*10.0f, Math::frand()*10.0f, Math::frand()*10.0f,
            Math::frand()*10.0f, Math::frand()*10.0f, Math::frand()*10.0f, Math::frand()*10.0f,
            Math::frand()*10.0f, Math::frand()*10.0f, Math::frand()*10.0f, Math::frand()*10.0f);

  Matrix4 rx1 = Matrix4::rotation_x(0.2451f);
  Matrix4 rx2 = Matrix4::rotation(0.2451f, 1.0f, 0.0f, 0.0f);

  Matrix4 ry1 = Matrix4::rotation_y(0.2451f);
  Matrix4 ry2 = Matrix4::rotation(0.2451f, 0.0f, 1.0f, 0.0f);

  Matrix4 rz1 = Matrix4::rotation_z(0.2451f);
  Matrix4 rz2 = Matrix4::rotation(0.2451f, 0.0f, 0.0f, 1.0f);

  Matrix4 tr = Matrix4::translate(12.0f, 13.5f, 15.0f);

  std::cout << "start:\n" << m << '\n'
            << "simple:\n" << s << '\n'
            << "determinant: " << m.determinant() << '\n'
            << "inverse:\n" << m.inverse() << '\n'
            << "mult:\n" << (m * m.inverse()) << '\n'
            << "transpose:\n" << (m.transpose()) << '\n'
            << "rotationX:\n" << rx1 << "\n\n" << rx2 << "\n\n"
            << "rotationY:\n" << ry1 << "\n\n" << ry2 << "\n\n"
            << "rotationZ:\n" << rz1 << "\n\n" << rz2 << "\n\n"
            << "translate:\n" << tr << "\n\n" << tr*tr << "\n\n"
            << "id-rot:\n" << Matrix4::rotation_x(0.987987f) * Matrix4::rotation_x(-0.987987f) << "\n\n"
    ;
  return 0;
}

/* EOF */
