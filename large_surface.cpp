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

#include <iostream>
#include "surface.hpp"
#include "large_surface.hpp"

const int max_texture_size = 1024;

void find_splice(int& rest, int& chunk)
{
  do
    {
      if (rest - chunk >= 0)
        {
          rest -= chunk;
          return;
        }
      else
        {
          chunk /= 2;
        }
    }
  while(chunk >= 16);
  
  // Don't create slices smaller then 16px
  chunk = rest;
  rest  = 0;
}

void make_splices(int width, std::vector<int>& slices)
{
  int chunk = max_texture_size;
  while(width > 0)
    {
      find_splice(width, chunk);
      slices.push_back(chunk);
    }
}

LargeSurface::LargeSurface(SWSurfaceHandle surface)
  : grid(1,1),
    width(surface->get_width()),
    height(surface->get_height())
{
  if (surface->get_width()  > max_texture_size ||
      surface->get_height() > max_texture_size)
    { // Large Surface, splice into small chunks
      std::vector<int> width_splices;
      make_splices(surface->get_width(), width_splices);

      std::vector<int> height_splices;
      make_splices(surface->get_height(), height_splices);

      grid.resize(width_splices.size(), height_splices.size());

      int x_pos = 0;
      int y_pos = 0;
      for(int iy = 0; iy < int(height_splices.size()); ++iy)
        {
          for(int ix = 0; ix < int(width_splices.size()); ++ix)
            {
              if (0)
                std::cout << x_pos << "+" << y_pos << " - " << width_splices[ix] << "x" << height_splices[iy] 
                          << " -> " << ix << "x" << iy 
                          << std::endl;
            
              grid(ix, iy) = new Surface(surface, x_pos, y_pos, 
                                         width_splices[ix], height_splices[iy]);
              x_pos += width_splices[ix];
            }
          x_pos = 0;
          y_pos += height_splices[iy];
        }
      if (0)
        std::cout << "------------------: " << grid.get_width() << "x" << grid.get_height() << std::endl;
    }
  else
    {
      grid(0,0) = new Surface(surface, 0, 0, 
                              surface->get_width(), 
                              surface->get_height());
    }
}

LargeSurface::~LargeSurface()
{
  for(int y = 0; y < grid.get_height(); ++y)
    for(int x = 0; x < grid.get_width(); ++x)
      delete grid(x,y);
}

void
LargeSurface::draw(float orig_x_pos, float y_pos, float w, float h)
{
  float x_pos = orig_x_pos;
  Surface* sur = 0;
  for(int y = 0; y < grid.get_height(); ++y)
    {
      for(int x = 0; x < grid.get_width(); ++x)
        {
          sur = grid(x,y);
          if (sur)
            {
              if (0)
              std::cout << "draw: " << x << "+" << y << " - " << w << "x" << h << " - " 
                        << sur->get_width() << "x" << sur->get_height() << " - " 
                        << get_width() << "x" << get_height() << std::endl;
              
              sur->draw(x_pos, y_pos, 
                        w * sur->get_width() / get_width(),
                        h * sur->get_height() / get_height());
              x_pos += w * sur->get_width() / get_width();
            }
        }
      if (sur)
        {
          y_pos += h * sur->get_height() / get_height();
          x_pos = orig_x_pos;
        }
    }
}

/* EOF */
