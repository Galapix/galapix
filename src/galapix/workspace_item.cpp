/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
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

#include "galapix/workspace_item.hpp"

WorkspaceItem::WorkspaceItem() :
  m_pos(),
  m_scale(),
  m_angle(),
  m_visible(false),
  m_parent(),
  m_children()
{
}

WorkspaceItem::~WorkspaceItem()
{
}

void
WorkspaceItem::set_pos(const Vector2f& pos)
{
  m_pos = pos;
}

Vector2f
WorkspaceItem::get_pos() const
{
  return m_pos;
}

Vector2f
WorkspaceItem::get_top_left_pos() const
{
  return m_pos - Vector2f(get_scaled_width()/2, get_scaled_height()/2);
}

void
WorkspaceItem::set_top_left_pos(const Vector2f& p)
{
  m_pos = p + Vector2f(get_scaled_width()/2, get_scaled_height()/2);
}

void
WorkspaceItem::set_angle(float a)
{
  m_angle = a;
}

float
WorkspaceItem::get_angle() const
{
  return m_angle;
}

void
WorkspaceItem::set_scale(float f)
{
  m_scale        = f;
}

float
WorkspaceItem::get_scale() const
{
  return m_scale;
}

float
WorkspaceItem::get_scaled_width() const
{
  return static_cast<float>(get_original_width()) * get_scale();
}

float
WorkspaceItem::get_scaled_height() const
{
  return static_cast<float>(get_original_height()) * get_scale();
}

Rectf
WorkspaceItem::get_image_rect() const
{
  return Rectf(get_top_left_pos(),
               Sizef(get_scaled_width(), get_scaled_height()));
}

/* EOF */
