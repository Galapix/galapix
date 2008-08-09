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

#include "math.hpp"
#include "workspace.hpp"

Workspace::Workspace()
{
}

void
Workspace::add_image(int fileid, const std::string& filename, const Size& size)
{
  images.push_back(Image(fileid, filename, size));
  //images.back().set_scale(Math::min(1024.0f / size.width,
  //                                  1024.0f / size.height));
}

void
Workspace::layout(float aspect_w, float aspect_h)
{
  if (!images.empty())
    {
      //       float x_pos = 0;
      //       for(Images::iterator i = images.begin(); i != images.end(); ++i)
      //         {
      //           i->set_pos(Vector2f(x_pos, 0.0f));
      //           x_pos += i->get_width() + 128/*spacing*/;
      //         }    
      
      int w = int(Math::sqrt(aspect_w * images.size() / aspect_h));

      for(int i = 0; i < int(images.size()); ++i)
        {
          if ((i/w) % 2 == 0)
            {
              images[i].set_pos(Vector2f((i % w) * 1024.0f,
                                         (i / w) * 1024.0f));
            }
          else
            {
              images[i].set_pos(Vector2f((w - (i % w)-1) * 1024.0f,
                                         (i / w)         * 1024.0f));
            }
        }
    }
}

void
Workspace::draw(const Rectf& cliprect, float scale)
{
  //std::cout << Math::clamp(1, static_cast<int>(1.0f / scale), 32) << " -> " << scale << std::endl;

  for(Images::iterator i = images.begin(); i != images.end(); ++i)
    {
      i->draw(cliprect, scale);
    }  
}

/* EOF */
