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

#include "blit.hpp"
#include "pixel_view.hpp"
#include "software_surface.hpp"
#include "unwrap.hpp"

namespace surf {

void blend_scaled(BlendFunc blendfunc, SoftwareSurface const& src, geom::irect const& srcrect, SoftwareSurface& dst, geom::irect const& dstrect)
{
  PIXELFORMAT2_TO_TYPE(
    src.get_format(), srctype,
    dst.get_format(), dsttype,
    blend_scaled_wrap(blendfunc,
                      src.as_pixelview<srctype>(), srcrect,
                      dst.as_pixelview<dsttype>(), dstrect));
}

void blend_scaled(BlendFunc blendfunc, SoftwareSurface const& src, SoftwareSurface& dst, geom::irect const& dstrect)
{
  blend_scaled(blendfunc, src, geom::irect(src.get_size()), dst, dstrect);
}

} // namespace surf

/* EOF */
