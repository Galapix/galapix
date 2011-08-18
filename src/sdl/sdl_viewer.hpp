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

#ifndef HEADER_GALAPIX_SDL_SDL_VIEWER_HPP
#define HEADER_GALAPIX_SDL_SDL_VIEWER_HPP

#include <SDL.h>
#include <memory>

#include "math/size.hpp"

#include "galapix/image.hpp"

class Viewer;
class FileEntry;
class Image;
class Workspace;

class SDLViewer
{
private:
  Size m_geometry;
  bool m_fullscreen;
  int  m_anti_aliasing;

  bool m_quit;
  bool m_spnav_allow_rotate;

  Viewer& m_viewer;
  
public:
  SDLViewer(const Size& geometry, bool fullscreen, int  anti_aliasing,
            Viewer& viewer);
  ~SDLViewer();

  void run();

private:
  void process_event(const SDL_Event& event);

private:
  SDLViewer (const SDLViewer&);
  SDLViewer& operator= (const SDLViewer&);
};

#endif

/* EOF */
