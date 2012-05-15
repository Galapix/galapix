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
public:
  WorkspaceItem();
  virtual ~WorkspaceItem();

  WorkspaceItemWPtr get_parent() const { return m_parent; }
  std::vector<WorkspaceItemPtr> get_children() const { return m_children; }

  virtual void     set_pos(const Vector2f& pos);
  virtual Vector2f get_pos() const;

  virtual Vector2f get_top_left_pos() const;
  virtual void set_top_left_pos(const Vector2f&);

  virtual void  set_scale(float f);
  virtual float get_scale() const;

  virtual void  set_angle(float a);
  virtual float get_angle() const;

  virtual float get_scaled_width()  const;
  virtual float get_scaled_height() const;

  virtual void refresh(bool force) = 0;

  virtual int get_original_width() const = 0;
  virtual int get_original_height() const = 0;

  virtual Rectf get_image_rect() const;
  
  virtual bool is_visible() const = 0;

  virtual void on_enter_screen() = 0;
  virtual void on_leave_screen() = 0;

  virtual void draw(const Rectf& cliprect, float zoom) = 0;
  virtual void draw_mark() = 0;

  virtual URL get_url() const = 0;

  virtual void clear_cache() = 0;
  virtual void cache_cleanup() = 0;
  virtual void print_info() const = 0;

private:
  /** Position refers to the center of the image */
  Vector2f m_pos;

  /** Scale of the image */
  float m_scale;

  /** Rotation angle */
  float m_angle;

  WorkspaceItemWPtr m_parent;
  std::vector<WorkspaceItemPtr> m_children;

private:
  WorkspaceItem(const WorkspaceItem&) = delete;
  WorkspaceItem& operator=(const WorkspaceItem&) = delete;
};

#endif

/* EOF */
