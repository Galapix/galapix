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

#ifndef HEADER_GALAPIX_GALAPIX_IMAGE_HPP
#define HEADER_GALAPIX_GALAPIX_IMAGE_HPP

#include <memory>
#include <map>
#include <string>

#include "galapix/tile_provider.hpp"
#include "galapix/tile.hpp"
#include "galapix/image_overview.hpp"
#include "job/job_handle.hpp"
#include "job/thread_message_queue2.hpp"
#include "math/rect.hpp"
#include "math/vector2f.hpp"
#include "util/url.hpp"
#include "galapix/workspace_item.hpp"
#include "math/rect.hpp"

namespace galapix {

class ImageTileCache;
class ImageRenderer;
class JobManager;
class Image;

class Image final : public WorkspaceItem
{
  friend class ImageRenderer;
public:
  Image(URL const& url, TileProviderPtr provider = {}, JobManager* job_manager = nullptr);
  ~Image() override;

  void prepare_tiles(Rectf const& cliprect, float zoom) override;
  /** Start provider jobs for tiles marked in prepare_tiles (budgeted). */
  void issue_tile_requests() override;
  /** Levels soft-underlay upgrade after grid jobs have taken budget. */
  void request_overview_levels(Rectf const& cliprect, float zoom);
  void draw(wstdisplay::GraphicsContext& gc, Rectf const& cliprect, float zoom) override;
  void draw_mark(wstdisplay::GraphicsContext& gc) override;

  // Used for sorting and debugging
  URL get_url() const override;

  int get_original_width() const override;
  int get_original_height() const override;

  // Debug stuff
  void clear_cache() override;
  void cache_cleanup() override;
  void print_info() const override;

  /** Outstanding tile jobs for this image (0 if no cache yet). */
  int pending_tile_requests() const;
  void dump_stuck_tile_requests(int limit = 16) const;
  /** Decoded tiles waiting for GL upload. */
  int pending_tile_uploads() const;
  int pending_upload_count() const override { return pending_tile_uploads(); }
  int tile_cache_entries() const;
  int ready_tile_surfaces() const;

  void on_leave_screen() override;

  ImageOverview& overview() { return *m_overview; }
  ImageOverview const& overview() const { return *m_overview; }
  ImageOverview::State overview_state() const {
    return m_overview ? m_overview->state() : ImageOverview::State::Idle;
  }
  JobManager* job_manager() const { return m_job_manager; }

private:
  void set_tile_provider(TileProviderPtr provider);
  TileProviderPtr get_tile_provider() const { return m_provider; }

private:
  URL       m_url;
  TileProviderPtr m_provider;
  JobManager* m_job_manager;

  std::shared_ptr<ImageTileCache> m_cache;
  std::unique_ptr<ImageRenderer>  m_renderer;
  std::shared_ptr<ImageOverview> m_overview;
};

} // namespace galapix

#endif

/* EOF */
