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

#include "galapix/regular_layouter.hpp"

#include "galapix/image_collection.hpp"
#include "galapix/image.hpp"
#include "math/math.hpp"

RegularLayouter::RegularLayouter(float aspect_w, float aspect_h) :
  m_aspect_w(aspect_w),
  m_aspect_h(aspect_h)
{
}

void
RegularLayouter::layout(const ImageCollection& images, bool animated)
{
  if (!images.empty())
  {
    int w = int(Math::sqrt(m_aspect_w * static_cast<float>(images.size()) / m_aspect_h));
      
    for(int i = 0; i < int(images.size()); ++i)
    {
      float target_scale = Math::min(1000.0f / static_cast<float>(images[i]->get_original_width()),
                                     1000.0f / static_cast<float>(images[i]->get_original_height()));

      images[i]->set_target_scale(target_scale);

      if ((i/w) % 2 == 0)
      {
        images[i]->set_target_pos(Vector2f(static_cast<float>(i % w) * 1024.0f,
                                           static_cast<float>(i / w) * 1024.0f));
      }
      else
      {
        images[i]->set_target_pos(Vector2f(static_cast<float>(w - (i % w)-1) * 1024.0f,
                                           static_cast<float>(i / w)         * 1024.0f));
      }
    }
  }
}

/* EOF */
