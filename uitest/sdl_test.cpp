/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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

#include <iostream>

#include "display/surface.hpp"
#include "math/vector2f.hpp"
#include "sdl/sdl_window.hpp"
#include "util/software_surface_factory.hpp"

int main(int argc, char** argv)
{
#if 0
  SoftwareSurfaceFactory factory;

  SDLWindow window(Size(800, 600), false, 0);
  
  for(int step = 0; step < 10000; ++step)
  {
    for(int i = 1; i < argc; ++i)
    {
      SurfacePtr surface = Surface::create(factory.from_file(argv[i]));
      surface->draw(Vector2f(static_cast<float>(rand()%800),
                             static_cast<float>(rand()%600)));
    }
    window.flip();
  }
  SDL_Delay(1000);
#endif
  return 0;
}

/* EOF */
