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

#ifndef HEADER_SURFACE_HPP
#define HEADER_SURFACE_HPP

#include "SDL.h"
#include <boost/smart_ptr.hpp>
#include "texture.hpp"
#include "math/size.hpp"
#include "software_surface.hpp"

class SurfaceImpl;
class Rect;
class Rectf;
class Vector2f;

class Surface
{
public:
  Surface();
  Surface(const SoftwareSurface& src, const Rect& srcrect);
  Surface(const SoftwareSurface& src);
  ~Surface();

  void draw(const Vector2f& pos);
  void draw(const Rectf& rect);

  int get_width()  const;
  int get_height() const;
  Size get_size() const;

  operator bool() const { return impl.get(); }

private:
  boost::shared_ptr<SurfaceImpl> impl;
};

#endif

/* EOF */
