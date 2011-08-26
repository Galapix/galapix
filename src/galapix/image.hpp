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

#ifndef HEADER_GALAPIX_GALAPIX_IMAGE_HPP
#define HEADER_GALAPIX_GALAPIX_IMAGE_HPP

#include <memory>
#include <map>
#include <string>

#include "database/file_entry.hpp"
#include "galapix/image_handle.hpp"
#include "galapix/tile_provider.hpp"
#include "galapix/tile.hpp"
#include "job/job_handle.hpp"
#include "job/thread_message_queue.hpp"
#include "math/rect.hpp"
#include "math/vector2f.hpp"
#include "util/url.hpp"

class ImageTileCache;
class ImageRenderer;
class TileEntry;
class Image;
class Rectf;


class Image
{
private:
  std::weak_ptr<Image> m_self;

  URL       m_url;
  TileProviderPtr m_provider;

  bool m_visible;
  Rectf m_image_rect;
  
  /** Position refers to the center of the image */
  Vector2f m_pos;
  Vector2f m_last_pos;
  Vector2f m_target_pos;

  /** Scale of the image */
  float m_scale;
  float m_last_scale;
  float m_target_scale;

  /** Rotation angle */
  float m_angle;

  bool m_file_entry_requested;

  std::shared_ptr<ImageTileCache> m_cache;
  std::unique_ptr<ImageRenderer>  m_renderer;

  ThreadMessageQueue<FileEntry> m_file_entry_queue;
  ThreadMessageQueue<Tile> m_tile_queue;
  ThreadMessageQueue<TileProviderPtr> m_tile_provider_queue;
  typedef std::vector<JobHandle> Jobs;
  Jobs m_jobs;

private:
  Image(const URL& url, TileProviderPtr provider);

  void set_weak_ptr(ImagePtr self);

public:
  static ImagePtr create(const URL& url, TileProviderPtr provider = TileProviderPtr());

public:
  ~Image();

  // _____________________________________________________
  // Drawing stuff
  void draw(const Rectf& cliprect, float zoom);
  void draw_mark();

  // _____________________________________________________
  // Update stuff
  void update_pos(float progress);

  // _____________________________________________________
  // Getter/Setter
  void set_target_pos(const Vector2f& target_pos);
  void set_target_scale(float target_scale);

  void     set_pos(const Vector2f& pos);
  Vector2f get_pos() const;

  void  set_scale(float f);
  float get_scale() const;

  void  set_angle(float a);
  float get_angle() const;

  void set_provider(TileProviderPtr provider);
  void clear_provider();

  // Used for sorting and debugging
  URL get_url() const;

  // _____________________________________________________
  // Getter
  float get_scaled_width() const;
  float get_scaled_height() const;

  int get_original_width() const;
  int get_original_height() const;

  Vector2f get_top_left_pos() const;

  // _____________________________________________________
  // Debug stuff
  void clear_cache();
  void refresh(bool force);
  void cache_cleanup();
  void print_info() const;

  // _____________________________________________________
  // Query Stuff
  bool overlaps(const Rectf& cliprect) const;
  bool overlaps(const Vector2f& pos) const;

  Rectf calc_image_rect() const;
  Rectf get_image_rect() const;

  bool is_visible() const { return m_visible; }

  void on_enter_screen();
  void on_leave_screen();

  void on_zoom_level_change();

  void abort_all_jobs();

public:
  /** Syncronized function to acquire data from other threads */
  void receive_file_entry(const FileEntry& file_entry);
  void receive_tile(const FileEntry& file_entry, const Tile& tile);
  void receive_tile_provider(TileProviderPtr provider);

private:
  void process_queues();
};

#endif

/* EOF */
