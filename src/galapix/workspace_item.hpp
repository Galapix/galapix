/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_GALAPIX_WORKSPACE_ITEM_HPP
#define HEADER_GALAPIX_GALAPIX_WORKSPACE_ITEM_HPP

#include <memory>
#include <vector>

class WorkspaceItem;

typedef std::weak_ptr<WorkspaceItem> WorkspaceItemWPtr;

class WorkspaceItem
{
private:
  WorkspaceItemWPtr m_parent;
  std::vector<WorkspaceItemPtr> m_children;

public:
  WorkspaceItem() :
    m_parent(),
    m_children()
  {}

  WorkspaceItemWPtr get_parent() const { return m_parent; }
  std::vector<WorkspaceItemPtr> get_children() const { return m_children; }

  void     set_pos(const Vector2f& pos);
  Vector2f get_pos() const;

  float get_width() const;
  float get_height() const;

private:
  WorkspaceItem(const WorkspaceItem&);
  WorkspaceItem& operator=(const WorkspaceItem&);
};

#endif

/* EOF */
