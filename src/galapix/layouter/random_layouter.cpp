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

#include "galapix/layouter/random_layouter.hpp"

#include "galapix/image.hpp"
#include "galapix/image_collection.hpp"

RandomLayouter::RandomLayouter()
{
}

void
RandomLayouter::layout(const ImageCollection& images)
{
  const int width = static_cast<int>(Math::sqrt(float(images.size())) * 1500.0f);

  for(ImageCollection::const_iterator i = images.begin(); i != images.end(); ++i)
  {
    (*i)->set_pos(Vector2f(static_cast<float>(rand() % width), 
                                  static_cast<float>(rand() % width)));

    // FIXME: Make this relative to image size
    (*i)->set_scale(static_cast<float>(rand()%1000) / 1000.0f + 0.25f); 
  }
}

/* EOF */
