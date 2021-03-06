/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_SPNAV_SPACE_NAVIGATOR_HPP
#define HEADER_GALAPIX_SPNAV_SPACE_NAVIGATOR_HPP

#include <thread>

class Viewer;

class SpaceNavigator
{
private:
  bool m_quit;
  std::thread m_thread;
  int m_pipefd[2];

public:
  SpaceNavigator();
  ~SpaceNavigator();

  void start_thread();
  void stop_thread();

private:
  void run();

private:
  SpaceNavigator (const SpaceNavigator&);
  SpaceNavigator& operator= (const SpaceNavigator&);
};

#endif

/* EOF */
