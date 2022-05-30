/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmail.com>
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
#include <stdexcept>
#include <uitest/uitest.hpp>

#include "util/failable.hpp"

using namespace galapix;

namespace {

class Foo
{
public:
  Foo() { std::cout << "Foo()" << std::endl; }
  ~Foo() { std::cout << "~Foo()" << std::endl; }
  Foo(const Foo& other) { std::cout << "Foo(const Foo&)" << std::endl; }
  Foo& operator=(const Foo& other) { std::cout << "Foo::operator=()" << std::endl; return *this; }
};

std::ostream& operator<<(std::ostream& os, const Foo& foo)
{
  return os << "<Foo()>";
}

} // namespace

UITEST(Failable, test, "")
{
  Failable<Foo> foo = Failable<Foo>::from_exception(std::runtime_error("My Exception"));
  if ((false))
  {
    foo.reset(Foo());
    foo.reset(Foo());
    //foo.set_exception();
  }
  try
  {
    std::cout << "Failable::get(): " << foo.get() << std::endl;
  }
  catch(const std::exception& err)
  {
    std::cout << "caught: " << err.what() << std::endl;
  }
}

/* EOF */
