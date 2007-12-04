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

#ifndef HEADER_WORKSPACE_HPP
#define HEADER_WORKSPACE_HPP

#include <vector>
#include "filesystem.hpp"
#include "griv.hpp"
#include "image.hpp"

class Workspace
{
public:
  std::vector<Image*> images;
  float res;
  bool   reorganize;
  Uint32 reorganize_start;
  float rotation;

public:
  Workspace();

  void draw();
  bool update_resources();
  void update(float delta);
  void add(const std::string& url);

  void zoom_in(int mouse_x, int mouse_y, float zoom);
  void zoom_out(int mouse_x, int mouse_y, float zoom);
  
  void set_zoom(float zoom);

  int size() { return images.size(); }
  void layout(int w, int h);
  void layout_random();
  void layout_sort(bool reverse);
};

#endif

/* EOF */
