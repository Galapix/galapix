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

#ifndef HEADER_WORKSPACE_HPP
#define HEADER_WORKSPACE_HPP

#include "url.hpp"
#include "selection.hpp"
#include "math/quad_tree.hpp"
#include "thread_message_queue.hpp"
#include "image.hpp"

class Rectf;

class ImageRequest
{
public:
  URL      url;
  Vector2f pos;
  float    scale;

public:
  ImageRequest(const URL&      url,
               const Vector2f& pos,
               float           scale)
    : url(url), pos(pos), scale(scale)
  {}
};

class Workspace
{
private:
  typedef std::vector<Image> Images;
  typedef std::vector<ImageRequest> ImageRequests;

  /** Simple QuadTree to have a quicker way to figure out which images
      are visible */
  std::auto_ptr<QuadTree<Image> > quad_tree;

  Images        images;
  ImageRequests image_requests;
  Selection     selection;

  // Layout hints
  // FIXME: ugly
  Vector2f  next_pos;
  int       row_width;

  /** Progress of the animation when relayouting, must be set to 0 to
      start animation */
  float     progress;

  ThreadMessageQueue<FileEntry>   file_queue;
 
public:
  Workspace();

  // ---------------------------------------------

  // Layout hints
  void set_row_width(int w);

  // Image order function
  void sort();
  void random_shuffle();

  void layout_tight(float aspect_w, float aspect_h);
  void layout_aspect(float aspect_w, float aspect_h);
  void layout_vertical();
  void layout_random();
  void solve_overlaps();

  // ---------------------------------------------

  // Image Query Functions (dangerous: need to now when Image position
  // changes) -> Return "const Image"? -> can't do breaks due to impl
  std::vector<Image> get_images(const Rectf& rect) const;
  Image get_image(const Vector2f& pos) const;

  // ---------------------------------------------

  void add_image(const FileEntry& file_entry); 
  void add_image(const FileEntry& file_entry, const Vector2f& pos, float scale); 
  void add_image(const URL& url, const Vector2f& pos, float scale);

  // Selection Commands
  Selection get_selection() const { return selection; }
  bool selection_clicked(const Vector2f& pos) const;
  void select_images(const std::vector<Image>& images);
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

  void build_quad_tree();
  void clear_quad_tree();

  Rectf get_bounding_rect() const;

  bool is_animated() const;

  void receive_file(const FileEntry& entry);

private:
  Workspace (const Workspace&);
  Workspace& operator= (const Workspace&);
};

#endif

/* EOF */
