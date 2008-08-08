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

#include "surface.hpp"
#include "image.hpp"

Image::Image(const std::string& filename, const Size& size)
  : tiles((size.width  + 255) / 256, 
          (size.height + 255) / 256)
{
}

void
Image::draw()
{
  for(int y = 0; y < tiles.get_height(); ++y)
    for(int x = 0; x < tiles.get_width(); ++x)
      {
        Surface* tile = tiles(x,y);
        if (tile)
          {
            tile->draw(Vector2f(x*256, y*256));
          }
      }

#if 0
  for(int y = 0; y*256 < entry.size.height/2; ++y)
    for(int x = 0; x*256 < entry.size.width/2; ++x)
      {
        Tile tile;
        if (tile_db.get_tile(entry.fileid, 1/*scale*/, x, y, tile))
          {
            std::cout << "Have tile: " << x << ", " << y << std::endl;
          }                
        else
          {                     
                    
          }
      }
#endif
}

/* EOF */
