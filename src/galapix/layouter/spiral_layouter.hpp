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

#ifndef HEADER_GALAPIX_GALAPIX_LAYOUTER_SPIRAL_LAYOUTER_HPP
#define HEADER_GALAPIX_GALAPIX_LAYOUTER_SPIRAL_LAYOUTER_HPP

#include "galapix/layouter/layouter.hpp"

#include "math/vector2i.hpp"

class WorkspaceItem;

class SpiralLayouter : public Layouter
{
private:
  enum Direction {
    kRight,
    kDown,
    kLeft,
    kUp
  };

private:
  Vector2i m_pos;
  int m_ring;
  Direction m_direction;

public:
  SpiralLayouter();

  void layout(const ImageCollection& images);
  void reset();
  void layout(WorkspaceItem& item);

private:
  void advance();

private:
  SpiralLayouter(const SpiralLayouter&);
  SpiralLayouter& operator=(const SpiralLayouter&);
};

#endif

/* EOF */

