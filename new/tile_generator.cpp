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
#include "math/size.hpp"
#include "math/rect.hpp"
#include "tile_database.hpp"
#include "tile_generator.hpp"

TileGenerator::TileGenerator()
{
}

TileGenerator::~TileGenerator()
{
}

void
TileGenerator::generate(int fileid, const SoftwareSurface& surface_, TileDatabase& tile_db)
{
  int scale = 0;

  SoftwareSurface surface = surface_;

  do
    {
      if (scale != 0)
        {
          surface = surface.scale(Size(surface.get_width()/2, 
                                       surface.get_height()/2));
        }

      for(int y = 0; 256*y < surface.get_height(); ++y)
        for(int x = 0; 256*x < surface.get_width(); ++x)
          {
            SoftwareSurface croped_surface = surface.crop(Rect(Vector2i(x * 256, y * 256),
                                                               Size(256, 256)));

            Tile tile;
            tile.fileid = fileid;
            tile.scale  = scale;
            tile.x = x;
            tile.y = y;
            tile.surface = croped_surface;
          
            tile_db.store_tile(tile);
          }

      scale += 1;

      std::cout << "Scale: " << scale << " - " << surface.get_size() << std::endl;
    } while (surface.get_width() > 32 ||
             surface.get_height() > 32);
}

/* EOF */
