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

#ifndef HEADER_GALAPIX_GALAPIX_WORKSPACE_HPP
#define HEADER_GALAPIX_GALAPIX_WORKSPACE_HPP

#include <set>

#include "galapix/image.hpp"
#include "galapix/image_collection.hpp"
#include "galapix/layouter/layouter.hpp"
#include "galapix/layouter/spiral_layouter.hpp"
#include "galapix/selection.hpp"
#include "galapix/workspace_item.hpp"
#include "job/thread_message_queue.hpp"
#include "math/quad_tree.hpp"
#include "util/url.hpp"

class Rectf;
class Layouter;

/** The Workspace houses all the images, the current selection and
    things like layouting of the image collection */
class Workspace final
{
public:
  Workspace();

  // Image order function
  void sort();
  void sort_reverse();
  void random_shuffle();

  void layout_spiral();
  void layout_tight(float aspect_w, float aspect_h);
  void layout_aspect(float aspect_w, float aspect_h);
  void layout_vertical();
  void layout_random();
  void solve_overlaps();

  // ---------------------------------------------

  // Image Query Functions (dangerous: need to now when Image position
  // changes) -> Return "const Image"? -> can't do breaks due to impl
  ImageCollection get_images(const Rectf& rect) const;
  WorkspaceItemPtr get_image(const Vector2f& pos) const;

  // ---------------------------------------------
  void add_image(const WorkspaceItemPtr& image);

  // Selection Commands
  SelectionPtr get_selection() const { return m_selection; }
  bool selection_clicked(const Vector2f& pos) const;
  void select_images(const ImageCollection& images);
  void clear_selection();
  void move_selection(const Vector2f& rel);
  void clear();

  /** Delete all images that aren't in the selection */
  void isolate_selection();

  /** Delete all images in the selection */
  void delete_selection();

  // ---------------------------------------------

  void draw(const Rectf& cliprect, float scale);
  void update(float delta);

  // ---------------------------------------------

  // Debug stuff
  void clear_cache();
  void cache_cleanup();
  void print_info(const Rectf& rect);
  void print_images(const Rectf& rect);

  // ---------------------------------------------
  void load(const std::string& filename);
  void save(std::ostream& out);

  Rectf get_bounding_rect() const;

private:
  void relayout();

private:
  ImageCollection m_images;
  SelectionPtr m_selection;
  LayouterPtr m_layouter;

private:
  Workspace (const Workspace&) = delete;
  Workspace& operator= (const Workspace&) = delete;
};

#endif

/* EOF */
