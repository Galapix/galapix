// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "galapix/layouter/spiral_layouter.hpp"

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
      if (m_pos.x() == last)
      {
        m_direction = kDown;
        m_pos = Vector2i(m_pos.x(),
                         m_pos.y() + 1);
      }
      else
      {
        m_pos = Vector2i(m_pos.x() + 1,
                         m_pos.y());
      }
      break;

    case kDown:
      if (m_pos.y() == last)
      {
        m_direction = kLeft;
        m_pos = Vector2i(m_pos.x() - 1,
                         m_pos.y());
      }
      else
      {
        m_pos = Vector2i(m_pos.x(),
                         m_pos.y() + 1);
      }
      break;

    case kLeft:
      if (m_pos.x() == first)
      {
        m_direction = kUp;
        m_pos = Vector2i(m_pos.x(),
                         m_pos.y() - 1);
      }
      else
      {
        m_pos = Vector2i(m_pos.x() - 1,
                         m_pos.y());
      }
      break;

    case kUp:
      if (m_pos.y() == first - 1)
      {
        m_direction = kRight;
        m_pos = Vector2i(m_pos.x() + 1,
                         m_pos.y());
        m_ring  += 1;
      }
      else
      {
        m_pos = Vector2i(m_pos.x(),
                         m_pos.y() - 1);
      }
      break;
  }
}

void
SpiralLayouter::layout(WorkspaceItem& item)
{
  // normalize the image size to 1000x1000
  float target_scale = Math::min(1000.0f / static_cast<float>(item.get_original_width()),
                                 1000.0f / static_cast<float>(item.get_original_height()));

  item.set_scale(target_scale);
  item.set_pos(Vector2f(static_cast<float>(m_pos.x()) * 1024.0f,
                               static_cast<float>(m_pos.y()) * 1024.0f));

  advance();
}

void
SpiralLayouter::layout(const ImageCollection& images)
{
  reset();
  for(const auto& i: images)
  {
    layout(*i);
  }
}

/* EOF */
