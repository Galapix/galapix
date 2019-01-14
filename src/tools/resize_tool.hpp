/*
**  Galapix - an image viewer for large image collections
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

#ifndef HEADER_GALAPIX_TOOLS_RESIZE_TOOL_HPP
#define HEADER_GALAPIX_TOOLS_RESIZE_TOOL_HPP

#include "galapix/tool.hpp"

class ResizeTool : public Tool
{
public:
  ResizeTool(Viewer* viewer);

  void move(const Vector2i& pos, const Vector2i& rel) override;
  void up(const Vector2i& pos) override;
  void down(const Vector2i& pos) override;

  void draw() override {}
  void update(const Vector2i& pos, float delta);

private:
  bool     resize_active;
  Vector2f resize_center;
  Vector2f selection_center;
  float    old_scale;
};

#endif

/* EOF */
