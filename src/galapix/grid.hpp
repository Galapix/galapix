// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_GALAPIX_GRID_HPP
#define HEADER_GALAPIX_GALAPIX_GRID_HPP

#include <iostream>
#include <assert.h>
#include <vector>

namespace galapix {

/** A std::vector like class that gives you a 2d grid into which you
    can place your elements: "grid(x,y) = value;"
 */
template<typename T>
class Grid
{
private:
  using Columns = std::vector<T>;
  using Rows = std::vector<Columns>;

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

  T const& operator()(int x, int y) const {
    assert(y >= 0 && y < rows.size());
    assert(x >= 0 && x < rows[y].size());
    return rows[y][x];
  }

  T& operator()(int x, int y) {
    assert(y >= 0 && y < int(rows.size()));
    assert(x >= 0 && x < int(rows[y].size()));
    return rows[y][x];
  }

  void resize(int w, int h, T const& t = T())
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

private:
  int width;
  int height;
  Rows rows;
};

} // namespace galapix

#endif

/* EOF */
