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

#include "file_entry.hpp"
#include "math.hpp"
#include "workspace.hpp"

Workspace::Workspace()
{
  next_pos = Vector2i(0, 0);
  row_width = 100;
}

void
Workspace::add_image(const FileEntry& file_entry)
{
  Image image(file_entry);
  images.push_back(image);
  image.set_scale(Math::min(1000.0f / file_entry.size.width,
                            1000.0f / file_entry.size.height));

  image.set_pos(next_pos * 1024.0f);

  // FIXME: Ugly, instead we should relayout once a second or so
  next_pos.x += 1;
  if (next_pos.x >= row_width)
    {
      next_pos.x  = 0;
      next_pos.y += 1;
    }
}

void
Workspace::layout(float aspect_w, float aspect_h)
{
  if (!images.empty())
    {     
      int w = int(Math::sqrt(aspect_w * images.size() / aspect_h));
      
      row_width = w;

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

          images[i].set_pos(images[i].get_pos() + Vector2f((1000.0f - images[i].get_scaled_width()) / 2,
                                                           (1000.0f - images[i].get_scaled_height()) / 2));

          next_pos = Vector2i(i % w, i / w);
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
