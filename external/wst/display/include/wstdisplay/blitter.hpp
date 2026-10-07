// SPDX-FileCopyrightText: 2002-2020 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_DISPLAY_BLITTER_HPP
#define HEADER_WINDSTILLE_DISPLAY_BLITTER_HPP

#include <surf/software_surface.hpp>

namespace wstdisplay {

/** Duplicate all the edge pixel of the given rectangle to the outside
    of the rectangle, thus creating a border around the given
    rectangle, this is needed for OpenGL textures to avoid filtering
    artefacts:

    X X X X X    1 1 2 3 3
    X 1 2 3 X _\ 1 1 2 3 3
    X 4 5 6 X  / 4 4 5 6 6
    X X X X X    4 4 5 6 6
*/
void generate_border(surf::SoftwareSurface& surface, int x_pos, int y_pos, int width, int height);

} // namespace wstdisplay

#endif

/* EOF */
