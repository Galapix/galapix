/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmx.de>
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

#include "util/log.hpp"

class Foobar
{
public:
  Foobar()
  {
    log_info << "info level log message" << std::endl;
    log_warning << "warning level log message" << std::endl;
    log_error << "info level log message" << std::endl;
    log_debug << "debug level log message" << std::endl;
  }

  void run(int i)
  {
    log_info << "info level log message" << std::endl;
    log_warning << "warning level log message" << std::endl;
    log_error << "info level log message" << std::endl;
    log_debug << "debug level log message" << std::endl;
  }
};

int main()
{
  log_info << "info level log message" << std::endl;
  log_warning << "warning level log message" << std::endl;
  log_error << "info level log message" << std::endl;
  log_debug << "debug level log message" << std::endl;

  Foobar foobar;
  foobar.run(5);

  return 0;
}

/* EOF */
