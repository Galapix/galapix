/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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
#include <memory>
#include <functional>
#include <uitest/uitest.hpp>

#include "util/weak_functor.hpp"

using namespace galapix;

class Foobar
{
public:
  void callme(int n)
  {
    std::cout << "Foobar::callme(" << n << ")" << std::endl;
  }

  void call()
  {
 std::cout << "Foobar::call()" << std::endl;
  }
};

UITEST(Signals, test, "")
{
  std::shared_ptr<Foobar> foobar(new Foobar);

  std::function<void (int)> callback  = weak(std::bind(&Foobar::callme, std::placeholders::_1, std::placeholders::_2), foobar);
  std::function<void ()> callback2 = weak(std::bind(&Foobar::call, std::placeholders::_1), foobar);

  callback(1323);

  //foobar.reset();
  //callback(1323);
  callback2();
}

/* EOF */
