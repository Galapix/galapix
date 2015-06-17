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

#include "math/rect.hpp"
#include "galapix/selection.hpp"

Selection::Selection() :
  m_images()
{
}

void
Selection::add_image(const WorkspaceItemPtr& image)
{
  m_images.add(image);
}

void
Selection::add_images(const ImageCollection& images)
{
  for(ImageCollection::const_iterator i = images.begin(); i != images.end(); ++i)
    m_images.add(*i);
}

void
Selection::remove_image(const WorkspaceItemPtr& image)
{
  m_images.remove(image);
}

void
Selection::scale(float factor)
{
  Vector2f center = get_center();
  for(ImageCollection::iterator i = m_images.begin(); i != m_images.end(); ++i)
  {
    (*i)->set_scale((*i)->get_scale() * factor);

    (*i)->set_pos(center + ((*i)->get_pos() - center) * factor);
  }
}

bool
Selection::has(const WorkspaceItemPtr& image) const
{
  for(ImageCollection::const_iterator i = m_images.begin(); i != m_images.end(); ++i)
  {
    if (image == *i)
    {
      return true;
    }
  }
  return false;
}

void
Selection::clear()
{
  m_images.clear();
}

ImageCollection
Selection::get_images() const
{
  return m_images;
}

bool
Selection::empty() const
{
  return m_images.empty();
}

Vector2f
Selection::get_center() const
{
  if (m_images.empty())
  {
    return Vector2f();
  }
  else
  {
    Vector2f pos;
    for(ImageCollection::const_iterator i = m_images.begin(); i != m_images.end(); ++i)
    {
      pos += (*i)->get_pos();
    }
    return pos / static_cast<float>(m_images.size());
  }
}

Selection::iterator
Selection::begin()
{
  return m_images.begin();
}

Selection::iterator
Selection::end()
{
  return m_images.end();
}

Selection::const_iterator
Selection::begin() const
{
  return m_images.begin();
}

Selection::const_iterator
Selection::end() const
{
  return m_images.end();
}

Rectf
Selection::get_bounding_rect() const
{
  if (m_images.empty())
  {
    return Rectf();
  }
  else
  {
    Rectf rect = m_images.front()->get_image_rect();

    for(ImageCollection::const_iterator i = m_images.begin()+1; i != m_images.end(); ++i)
    {
      const Rectf& image_rect = (*i)->get_image_rect();

      rect.left   = Math::min(rect.left,   image_rect.left);
      rect.right  = Math::max(rect.right,  image_rect.right);
      rect.top    = Math::min(rect.top,    image_rect.top);
      rect.bottom = Math::max(rect.bottom, image_rect.bottom);

      if (isnan(rect.left) ||
          isnan(rect.right) ||
          isnan(rect.top) ||
          isnan(rect.bottom))
      {
        //std::cout << i->get_url() << " " << i->get_pos() << " " << image_rect << std::endl;
        assert(false && "NAN Rect encountered");
      }
    }

    return rect;
  }
}

/* EOF */
