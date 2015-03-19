/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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

#include "util/task.hpp"

#include <stdexcept>
#include <functional>
#include <iostream>
#include <uitest/uitest.hpp>

UITEST(Task, test, "")
{
  Task<int> task{[]{
      std::cout << "doing a thing" << std::endl;
      throw std::runtime_error("broken");
      return 5;
    }};

  task();
  std::cout << "still ok" << std::endl;
  try
  {
    std::cout << "value: " << task.get() << std::endl;
  }
  catch(const std::exception& err)
  {
    std::cout << "catched error" << std::endl;
  }
}

/* EOF */
