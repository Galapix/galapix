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

#include "galapix/spiral_layouter.hpp"

#include "galapix/image.hpp"
#include "galapix/image_collection.hpp"
#include "math/math.hpp"

SpiralLayouter::SpiralLayouter() :
  m_pos(1, 0),
  m_ring(0),
  m_direction(kDown)
{
}

void
SpiralLayouter::reset()
{
  m_pos = Vector2i(1, 0);
  m_ring = 0;
  m_direction = kDown;
}

void
SpiralLayouter::advance()
{
  const int first = -m_ring;
  const int last  = m_ring + 1;

  switch(m_direction)
  {
    case kRight:
      if (m_pos.x == last)
      {
        m_direction = kDown;
        m_pos.y += 1;
      }
      else 
      {
        m_pos.x += 1;
      }
      break;

    case kDown:
      if (m_pos.y == last)
      {
        m_direction = kLeft;
        m_pos.x -= 1;
      }
      else 
      {
        m_pos.y += 1;
      }
      break;

    case kLeft:
      if (m_pos.x == first)
      {
        m_direction = kUp;
        m_pos.y -= 1;
      }
      else 
      {
        m_pos.x -= 1;
      }
      break;

    case kUp:
      if (m_pos.y == first - 1)
      {
        m_direction = kRight;
        m_pos.x += 1;
        m_ring  += 1;
      }
      else 
      {
        m_pos.y -= 1;
      }
      break;
  }
}

void
SpiralLayouter::layout(Image& image, bool animated)
{
  // normalize the image size to 1000x1000
  float target_scale = Math::min(1000.0f / static_cast<float>(image.get_original_width()),
                                 1000.0f / static_cast<float>(image.get_original_height()));

  if (animated)
  {
    image.set_target_scale(target_scale);
    image.set_target_pos(Vector2f(static_cast<float>(m_pos.x) * 1024.0f,
                                  static_cast<float>(m_pos.y) * 1024.0f));
  }
  else
  {
    image.set_scale(target_scale);
    image.set_pos(Vector2f(static_cast<float>(m_pos.x) * 1024.0f,
                           static_cast<float>(m_pos.y) * 1024.0f));
  }

  advance();
}

void
SpiralLayouter::layout(const ImageCollection& images, bool animated)
{
  reset();
  for(ImageCollection::const_iterator i = images.begin(); i != images.end(); ++i)
  {
    layout(**i, animated);
  }
}

/* EOF */
