/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#ifndef HEADER_GRID_HPP
#define HEADER_GRID_HPP

#include <vector>

/** */
template<typename T>
class Grid
{
private:
  typedef std::vector<T> Columns;
  typedef std::vector<Columns> Rows;

  int width;
  int height;
  Rows rows;

public:
  Grid(int w, int h) 
    : width(w),
      height(h)
  {
    for(int y = 0; y < height; ++y)
      rows.push_back(Columns(width));
  }
  
  int get_width()  const { return width; }
  int get_height() const { return height; }

  const T& operator()(int x, int y) const {
    assert(y >= 0 && y < rows.size());
    assert(x >= 0 && x < rows[y].size());
    return rows[y][x];
  }

  T& operator()(int x, int y) {
    assert(y >= 0 && y < int(rows.size()));
    assert(x >= 0 && x < int(rows[y].size()));
    return rows[y][x];
  }

  void resize(int w, int h, const T& t = T())
  {
    for(typename Rows::iterator i = rows.begin(); i != rows.end(); ++i)
      i->resize(w);
    rows.resize(h, Columns(w, t));

    if (0)
      {
        std::cout << "Resize: " << w << "x" << h << std::endl;
        std::cout << "Rows: " << rows.size() << std::endl;
        for(int y = 0; y < int(rows.size()); ++y)
          {
            std::cout << "Column: " << y << " -> " << rows[y].size() << std::endl;
          }
      }

    width  = w;
    height = h;
  }
};

#endif

/* EOF */
