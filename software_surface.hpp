/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#ifndef HEADER_SOFTWARE_SURFACE_HPP
#define HEADER_SOFTWARE_SURFACE_HPP

#include "SDL.h"
#include <string>
#include <boost/shared_ptr.hpp>

/** */
class SoftwareSurface
{
private:
  SDL_Surface* surface;
  
public:
  SoftwareSurface(const std::string& name);
  SoftwareSurface(SDL_Surface* swsurface);
  ~SoftwareSurface();

  int get_width() const { return surface->w; }
  int get_height() const { return surface->h; }
 
  SDL_Surface* get_surface() const { return surface; }
private:
  SoftwareSurface(const SoftwareSurface&);
  SoftwareSurface& operator= (const SoftwareSurface&);
};

typedef boost::shared_ptr<SoftwareSurface> SWSurfaceHandle;

#endif

/* EOF */
