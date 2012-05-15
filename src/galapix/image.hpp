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

#include "galapix/tile_provider.hpp"
#include "galapix/tile.hpp"
#include "job/job_handle.hpp"
#include "job/thread_message_queue2.hpp"
#include "math/rect.hpp"
#include "math/vector2f.hpp"
#include "util/url.hpp"
#include "galapix/workspace_item.hpp"

// error: base class 'class std::enable_shared_from_this<Image>' has a non-virtual destructor
#pragma GCC diagnostic ignored "-Weffc++"

class ImageTileCache;
class ImageRenderer;
class TileEntry;
class Image;
class Rectf;

class Image final : public WorkspaceItem,
                    public std::enable_shared_from_this<Image>
{
public:
  Image(const URL& url, TileProviderPtr provider = TileProviderPtr());
  virtual ~Image();

  void draw(const Rectf& cliprect, float zoom) override;
  void draw_mark() override;

  // Used for sorting and debugging
  URL get_url() const override;

  int get_original_width() const override;
  int get_original_height() const override;

  // _____________________________________________________
  // Debug stuff
  void clear_cache();
  void refresh(bool force);
  void cache_cleanup();
  void print_info() const;

  bool is_visible() const { return m_visible; }

  void on_enter_screen();
  void on_leave_screen();

  void on_zoom_level_change();

  void abort_all_jobs();

  /** Syncronized function to acquire data from other threads */
  void receive_tile_provider(TileProviderPtr provider);

private:
  void set_provider(TileProviderPtr provider);
  void process_queues();

private:
  URL       m_url;
  TileProviderPtr m_provider;

  bool m_visible;
  
  std::shared_ptr<ImageTileCache> m_cache;
  std::unique_ptr<ImageRenderer>  m_renderer;

  ThreadMessageQueue2<TileProviderPtr> m_tile_provider_queue;
  std::vector<JobHandle> m_jobs;
};

#endif

/* EOF */
