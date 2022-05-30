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

#ifndef HEADER_GALAPIX_TOOLS_VIEW_ROTATE_TOOL_HPP
#define HEADER_GALAPIX_TOOLS_VIEW_ROTATE_TOOL_HPP

#include "galapix/tool.hpp"

namespace galapix {

class ViewRotateTool : public Tool
{
public:
  ViewRotateTool(Viewer* viewer);

  void move(Vector2i const& pos, Vector2i const& rel) override;
  void up(Vector2i const& pos) override;
  void down(Vector2i const& pos) override;

  void draw(wstdisplay::GraphicsContext& gc) override {}
  void update(Vector2i const& /*pos*/, float /*delta*/) {}

private:
  bool active;
  float start_angle;
};

} // namespace galapix

#endif

/* EOF */
