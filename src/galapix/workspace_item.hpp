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

#include "math/rect.hpp"
#include "util/url.hpp"

class Vector2f;
class WorkspaceItem;

typedef std::weak_ptr<WorkspaceItem>   WorkspaceItemWPtr;
typedef std::shared_ptr<WorkspaceItem> WorkspaceItemPtr;

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

  virtual ~WorkspaceItem()
  {}

  WorkspaceItemWPtr get_parent() const { return m_parent; }
  std::vector<WorkspaceItemPtr> get_children() const { return m_children; }

  virtual void     set_pos(const Vector2f& pos) = 0;
  virtual Vector2f get_pos() const = 0;

  virtual float get_scaled_width()  const = 0;
  virtual float get_scaled_height() const = 0;

  virtual void refresh(bool force) = 0;

  // used by layouter
  virtual void set_target_pos(const Vector2f& target_pos) = 0;
  virtual void set_target_scale(float target_scale) = 0;

  virtual void  set_scale(float f) = 0;
  virtual float get_scale() const = 0;

  virtual void  set_angle(float a) = 0;
  virtual float get_angle() const = 0;

  virtual int get_original_width() const = 0;
  virtual int get_original_height() const = 0;

  virtual Rectf get_image_rect() const = 0;

  virtual bool overlaps(const Rectf& cliprect) const = 0;
  virtual bool overlaps(const Vector2f& pos) const = 0;
  
  virtual bool is_visible() const = 0;


  virtual void on_enter_screen() = 0;
  virtual void on_leave_screen() = 0;

  virtual void draw(const Rectf& cliprect, float zoom) = 0;
  virtual void draw_mark() = 0;

  virtual URL get_url() const = 0;

  virtual void clear_cache() = 0;
  virtual void cache_cleanup() = 0;
  virtual void print_info() const = 0;
  virtual void update_pos(float progress) = 0;

private:
  WorkspaceItem(const WorkspaceItem&) = delete;
  WorkspaceItem& operator=(const WorkspaceItem&) = delete;
};

#endif

/* EOF */
