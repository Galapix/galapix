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

#ifndef HEADER_VIEWER_HPP
#define HEADER_VIEWER_HPP

#include "SDL.h"
#include "surface.hpp"
#include "math/vector2i.hpp"

class Workspace;

class ViewerState
{
private:
  float    scale;
  Vector2f offset;
  
public:
  ViewerState();

  void zoom(float factor, const Vector2i& pos);
  void move(const Vector2i& pos);

  Vector2f screen2world(const Vector2i&) const;
  Rectf    screen2world(const Rect&) const;

  Vector2f get_offset() const { return offset; }
  float    get_scale()  const { return scale; }
};

class Viewer
{
private:
  bool quit;
  bool force_redraw;
  bool drag_n_drop;
  int  zoom_button;
  float gamma;

  Vector2i mouse_pos;

  ViewerState state;

public:
  Viewer();

  void draw(Workspace& workspace);
  void update(float delta);
  void process_event(const SDL_Event& event);
  bool done() const { return quit; }

private:
  Viewer (const Viewer&);
  Viewer& operator= (const Viewer&);
};

#endif

/* EOF */
