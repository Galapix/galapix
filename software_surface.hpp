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

#include <boost/shared_ptr.hpp>
#include "FreeImage.h"

class URL;
class Rect;
class Size;
class SoftwareSurfaceImpl;

class SoftwareSurface
{
public:
  SoftwareSurface();
  SoftwareSurface(const std::string& filename);
  SoftwareSurface(FIBITMAP* bitmap);
  ~SoftwareSurface();

  int get_width()  const;
  int get_height() const;
  int get_pitch()  const;

  SoftwareSurface scale(const Size& size) const;
  SoftwareSurface crop(const Rect& rect) const;

  void save(const std::string& filename) const;

  std::string get_data() const;

private:
  boost::shared_ptr<SoftwareSurfaceImpl> impl;
};

#endif

/* EOF */
