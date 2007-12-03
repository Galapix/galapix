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

#ifndef HEADER_IMAGE_HPP
#define HEADER_IMAGE_HPP

#include <sstream>
#include "display.hpp"
#include "loader.hpp"
#include "griv.hpp"

class Surface;
class SoftwareSurface;
class LargeSurface;

class Image
{
public:
  std::string url;
  int original_width;
  int original_height;
  unsigned int original_mtime;

  int  requested_res;

  /** Newly received surface */
  SoftwareSurface* received_surface;
  int              received_surface_res;
  
  int          surface_resolution;
  LargeSurface* surface;
  LargeSurface* surface_16x16;

  SDL_mutex* mutex;

private:
  float x_pos;
  float y_pos;

  float last_x_pos;
  float last_y_pos;

  float target_x_pos;
  float target_y_pos;

  bool visible;

public:
  Image(const std::string& url);
  ~Image();

  void receive(SoftwareSurface* new_surface, int r);
  void draw(float x_offset, float y_offset, float zoom);
  bool update_resources(float x_offset, float y_offset, float zoom);
  void update(float delta);

  int  zoom2res(float res);
  void set_pos(float x, float y);

  bool is_visible() const { return visible; }
};

#endif

/* EOF */
