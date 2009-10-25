/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "galapix/tight_layouter.hpp"

#include "galapix/image.hpp"
#include "galapix/image_collection.hpp"

TightLayouter::TightLayouter(float w, float h) :
  m_aspect_w(w),
  m_aspect_h(h)
{
}

void
TightLayouter::layout(const ImageCollection& images, bool animated)
{
  float spacing = 24.0f;

  float width = 0;  
  // calculate the total width 
  for(ImageCollection::const_iterator i = images.begin(); i != images.end(); ++i)
  {
    const float scale = (1000.0f + spacing) / static_cast<float>((*i)->get_original_height());
    width += static_cast<float>((*i)->get_original_width()) * scale;
  }

  width /= Math::sqrt(width / ((m_aspect_w / m_aspect_h) * (1000.0f + spacing)));

  Vector2f pos(0.0f, 0.0f);
  Vector2f last_pos(0.0f, 0.0f);
  bool go_right = true;
  
  for(ImageCollection::const_iterator i = images.begin(); i != images.end(); ++i)
  {
    const ImagePtr& image = *i;

    const float scale = 1000.0f / static_cast<float>(image->get_original_height());
    image->set_target_scale(scale);

    if (go_right)
    {
      if (pos.x + (static_cast<float>(image->get_original_width())*scale) > width)
      {
        pos.x = last_pos.x;
        pos.y += 1000.0f + spacing;   
              
        go_right = false;

        last_pos = pos;
        image->set_target_pos(pos + Vector2f(static_cast<float>(image->get_original_width()),
                                             static_cast<float>(image->get_original_height())) * scale / 2.0f);
      }
      else
      {
        last_pos = pos;
        image->set_target_pos(pos + Vector2f(static_cast<float>(image->get_original_width()),
                                             static_cast<float>(image->get_original_height())) * scale / 2.0f);
            
        pos.x += static_cast<float>(image->get_original_width()) * scale + spacing;
      }
    }
    else
    {
      if (pos.x - (static_cast<float>(image->get_original_width()) * scale) < 0)
      {
        //pos.x = 0;
        pos.y += 1000.0f + spacing;   
        go_right = true;

        last_pos = pos;
        image->set_target_pos(pos + Vector2f(static_cast<float>(image->get_original_width()),
                                             static_cast<float>(image->get_original_height())) * scale / 2.0f);

        pos.x += static_cast<float>(image->get_original_width()) * scale + spacing;
      }
      else
      {
        pos.x -= static_cast<float>(image->get_original_width()) * scale + spacing;

        last_pos = pos;
        image->set_target_pos(pos + Vector2f(static_cast<float>(image->get_original_width()),
                                             static_cast<float>(image->get_original_height())) * scale / 2.0f);
      }
    }
  }
}

/* EOF */
