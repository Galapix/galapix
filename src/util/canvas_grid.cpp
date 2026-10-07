// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "util/canvas_grid.hpp"

#include <cmath>

#include <wstdisplay/canvas.hpp>

namespace galapix {

void
draw_grid(wstdisplay::Canvas& canvas, geom::fpoint const& offset, geom::fsize const& size,
          surf::Color const& color, geom::fsize const& area)
{
  if (size.width() <= 0.0f || size.height() <= 0.0f) {
    return;
  }

  float const start_x = std::fmod(offset.x(), size.width());
  float const start_y = std::fmod(offset.y(), size.height());

  for (float x = start_x; x < area.width(); x += size.width()) {
    canvas.draw_line(geom::fpoint(x, 0.0f), geom::fpoint(x, area.height()), color);
  }

  for (float y = start_y; y < area.height(); y += size.height()) {
    canvas.draw_line(geom::fpoint(0.0f, y), geom::fpoint(area.width(), y), color);
  }
}

} // namespace galapix

/* EOF */
