// surf - Software surface library
// Copyright (C) 2008-2020 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <string.h>

#include "fill.hpp"
#include "software_surface.hpp"
#include "unwrap.hpp"

namespace surf {

void fill_rect(SoftwareSurface& dst, geom::irect const& rect, Color const& color)
{
  PIXELFORMAT_TO_TYPE(
    dst.get_format(), dsttype,
    fill_rect(dst.as_pixelview<dsttype>(), rect, convert<Color, dsttype>(color)));
}

void fill(SoftwareSurface& dst, Color const& color)
{
  fill_rect(dst, geom::irect(dst.get_size()), color);
}

void fill_checkerboard(SoftwareSurface& dst, geom::isize const& size,
                       Color const& color)
{
  PIXELFORMAT_TO_TYPE(
    dst.get_format(), dsttype,
    fill_checkerboard(dst.as_pixelview<dsttype>(), size,
                      convert<Color, dsttype>(color)));
}

} // namespace surf

/* EOF */
