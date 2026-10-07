// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_UTIL_CANVAS_GRID_HPP
#define HEADER_GALAPIX_UTIL_CANVAS_GRID_HPP

#include <geom/point.hpp>
#include <geom/size.hpp>
#include <surf/color.hpp>
#include <wstdisplay/fwd.hpp>

namespace galapix {

/** Draw grid lines with cells of \a size, shifted by \a offset,
    covering (0,0) to \a area in the current canvas coordinates
    (replaces the former wstdisplay::GraphicsContext::draw_grid()) */
void draw_grid(wstdisplay::Canvas& canvas, geom::fpoint const& offset, geom::fsize const& size,
               surf::Color const& color, geom::fsize const& area);

} // namespace galapix

#endif

/* EOF */
