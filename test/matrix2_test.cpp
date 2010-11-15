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
#include "math/matrix2.hpp"

int main(int argc, char* argv[])
{
  //Matrix2 m(1.0f, 2.0f,
  //          3.0f, 4.0f);
  srand(time(NULL));
  
  Matrix2 m(Math::frand()*10.0f, Math::frand()*10.0f,
            Math::frand()*10.0f, Math::frand()*10.0f);
  Vector2f v(Math::frand()*10.0f, Math::frand()*10.0f);
  Matrix2 r1 = Matrix2::rotation(Math::pi/2);
  Matrix2 r2 = Matrix2::rotation(Math::pi/2);

  std::cout << "start:\n" << m << '\n'
            << "determinant: " << m.determinant() << '\n'
            << "inverse:\n" << m.inverse() << '\n'
            << "mult:\n" << (m * m.inverse()) << '\n'
            << "transpose:\n" << (m.transpose()) << '\n'
            << "vec: " << v << '\n'
            << "rot1: " << r1 << '\n'
            << "rot2: " << r2 << '\n'
            << "vecmult: " << r1* r2 * v << '\n';

  return 0;
}

/* EOF */
