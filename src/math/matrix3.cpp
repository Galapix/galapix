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
#include <boost/format.hpp>

#include "math/matrix3.hpp"

std::ostream& operator<<(std::ostream& s, const Matrix3& m)
{
  return s << boost::format(",-                      -.\n"
                            "| %6.2f, %6.2f, %6.2f |\n"
                            "| %6.2f, %6.2f, %6.2f |\n"
                            "| %6.2f, %6.2f, %6.2f |\n"
                            "'-                      -'")
    % m(0,0) % m(0,1) % m(0,2) 
    % m(1,0) % m(1,1) % m(1,2) 
    % m(2,0) % m(2,1) % m(2,2) ;
}

/* EOF */
