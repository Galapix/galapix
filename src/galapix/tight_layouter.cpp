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
  m_pos(0.0f, 0.0f),
  m_last_pos(0.0f, 0.0f),
  m_go_right(true),
  m_aspect_w(w),
  m_aspect_h(h)
{
}

void
TightLayouter::reset()
{
  m_pos      = Vector2f(0.0f, 0.0f);
  m_last_pos = Vector2f(0.0f, 0.0f);
  m_go_right = true;
}

void
TightLayouter::set_width(float width)
{
  m_width = width;
}

void
TightLayouter::layout(Image& image, bool animated)
{
  const float spacing = 24.0f;
  const float scale = 1000.0f / static_cast<float>(image.get_original_height());

  image.set_target_scale(scale);

  if (m_go_right)
  {
    if (m_pos.x + (static_cast<float>(image.get_original_width()) * scale) > m_width)
    {
      m_pos.x = m_last_pos.x;
      m_pos.y += 1000.0f + spacing;   
              
      m_go_right = false;

      m_last_pos = m_pos;
      image.set_target_pos(m_pos + Vector2f(static_cast<float>(image.get_original_width()),
                                          static_cast<float>(image.get_original_height())) * scale / 2.0f);
    }
    else
    {
      m_last_pos = m_pos;
      image.set_target_pos(m_pos + Vector2f(static_cast<float>(image.get_original_width()),
                                            static_cast<float>(image.get_original_height())) * scale / 2.0f);
            
      m_pos.x += static_cast<float>(image.get_original_width()) * scale + spacing;
    }
  }
  else
  {
    if (m_pos.x - (static_cast<float>(image.get_original_width()) * scale) < 0)
    {
      //m_pos.x = 0;
      m_pos.y += 1000.0f + spacing;   
      m_go_right = true;

      m_last_pos = m_pos;
      image.set_target_pos(m_pos + Vector2f(static_cast<float>(image.get_original_width()),
                                            static_cast<float>(image.get_original_height())) * scale / 2.0f);

      m_pos.x += static_cast<float>(image.get_original_width()) * scale + spacing;
    }
    else
    {
      m_pos.x -= static_cast<float>(image.get_original_width()) * scale + spacing;

      m_last_pos = m_pos;
      image.set_target_pos(m_pos + Vector2f(static_cast<float>(image.get_original_width()),
                                            static_cast<float>(image.get_original_height())) * scale / 2.0f);
    }
  }
}

void
TightLayouter::layout(const ImageCollection& images, bool animated)
{
  const float spacing = 1024.0f;
  float width = 0;  

  // calculate the total width 
  for(ImageCollection::const_iterator i = images.begin(); i != images.end(); ++i)
  {
    const float scale = (1000.0f + spacing) / static_cast<float>((*i)->get_original_height());
    width += static_cast<float>((*i)->get_original_width()) * scale;
  }
  
  width /= Math::sqrt(width / ((m_aspect_w / m_aspect_h) * (1000.0f + spacing)));

  set_width(width);

  // do final layouting
  reset();
  for(ImageCollection::const_iterator i = images.begin(); i != images.end(); ++i)
  {
    layout(**i, animated);
  }
}

/* EOF */
