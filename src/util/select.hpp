/*
**  Xbox360 USB Gamepad Userspace Driver
**  Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_UTIL_SELECT_HPP
#define HEADER_GALAPIX_UTIL_SELECT_HPP

#include <sys/select.h>

/** Little class around select(), not full featured, just readfds are
    supported right nowx */
class Select
{
public:
  Select();

  void clear();
  void add_fd(int fd);
  bool is_ready(int fd) const;

  int wait();

private:
  int m_nfds;
  fd_set m_readfds;

private:
  Select(const Select&);
  Select& operator=(const Select&);
};

#endif

/* EOF */
