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

#ifndef HEADER_GRIV_HPP
#define HEADER_GRIV_HPP

#include <SDL.h>
#include <string>

extern float x_offset;
extern float y_offset;
extern bool force_redraw;
extern bool highquality;

class Workspace;

class Griv
{
private:
  bool  drag_n_drop;
  float old_res;
  float old_x_offset;
  float old_y_offset;
  Uint32 next_redraw;
  Workspace* workspace;
  int zoom_mode;

  bool zoom_in_pressed;
  bool zoom_out_pressed;
  int  mouse_x;
  int  mouse_y;

public:
  Griv();
  ~Griv();
  
  void process_events(float delta);
  int main(int argc, char** argv);
};

#endif

/* EOF */
