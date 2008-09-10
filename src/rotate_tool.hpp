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

#ifndef HEADER_ROTATE_TOOL_HPP
#define HEADER_ROTATE_TOOL_HPP

#include "tool.hpp"

class RotateTool : public Tool
{
private:
  bool     rotate_active;
  Vector2f selection_center;
  float    start_angle;

public:
  RotateTool(Viewer* viewer);

  void move(const Vector2i& pos, const Vector2i& rel);
  void up  (const Vector2i& pos);
  void down(const Vector2i& pos);

  void draw() {}
  void update(const Vector2i& pos, float delta);
};

#endif

/* EOF */
