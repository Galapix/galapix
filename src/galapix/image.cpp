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

#include "galapix/image.hpp"

#include <functional>
#include <iostream>

#include <surf/color.hpp>
#include <wstdisplay/graphics_context.hpp>

#include "galapix/image_renderer.hpp"
#include "galapix/image_tile_cache.hpp"
#include "galapix/viewer.hpp"
#include "util/weak_functor.hpp"

namespace galapix {

using namespace surf;

Image::Image(URL const& url, TileProviderPtr provider, JobManager* job_manager) :
  m_url(url),
  m_provider(std::move(provider)),
  m_job_manager(job_manager),
  m_cache(),
  m_renderer(),
  m_overview(std::make_shared<ImageOverview>())
{
  set_tile_provider(m_provider);
}

Image::~Image()
{
  if (m_overview) {
    m_overview->clear();
  }
}

int
Image::get_original_width() const
{
  if (m_provider)
  {
    return m_provider->get_size().width();
  }
  else
  {
    return 256;
  }
}

int
Image::get_original_height() const
{
  if (m_provider)
  {
    return m_provider->get_size().height();
  }
  else
  {
    return 256;
  }
}

void
Image::clear_cache()
{
  if (m_cache)
  {
    m_cache->clear();
  }
  if (m_overview) {
    m_overview->clear();
  }
}

void
Image::cache_cleanup()
{
  if (m_cache)
  {
    m_cache->cleanup();
  }
}

void
Image::prepare_tiles(Rectf const& cliprect, float zoom)
{
  if (!m_provider || !m_cache || !m_renderer) {
    return;
  }
  if (m_overview) {
    m_overview->process();
  }
  m_cache->process_queue();
  // Mark only — Workspace issues after every image has marked so the
  // global request budget is not monopolized by early items in m_images.
  m_renderer->prepare(cliprect, zoom);
}

void
Image::issue_tile_requests()
{
  if (m_cache) {
    m_cache->issue_requests();
  }
}

void
Image::draw(wstdisplay::GraphicsContext& gc, Rectf const& cliprect, float zoom)
{
  if (!m_provider)
  {
    gc.fill_rect(Rectf(get_top_left_pos(), Sizef(get_scaled_width(), get_scaled_height())),
                 surf::Color::from_rgb888(255,255,0));
  }
  else
  {
    // Uploads and requests were handled in prepare_tiles; draw is pure.
    m_renderer->draw(gc, cliprect, zoom);
  }
}

void
Image::set_tile_provider(TileProviderPtr provider)
{
  float old_size = static_cast<float>(std::max(get_original_width(),
                                               get_original_height()));

  // cleanup the old provider if present
  if (m_provider)
  {
    m_cache.reset();
    m_renderer.reset();
    m_provider.reset();
  }

  // set the new provider and related data
  if (provider)
  {
    m_provider = std::move(provider);
    m_cache    = std::make_shared<ImageTileCache>(m_provider);
    m_cache->prefetch_overview();
    m_renderer = std::make_unique<ImageRenderer>(*this, m_cache);
  }

  // Fixup the scale to fit into the old constrains more or less (only
  // works with regular layouts)
  float new_size = static_cast<float>(std::max(get_original_width(),
                                               get_original_height()));
  set_scale(get_scale() * old_size / new_size);
}

void
Image::print_info() const
{
  std::cout << "  Image: " << this << std::endl;
  if (m_cache) {
    std::cout << "    Tile cache entries: " << m_cache->cache_entry_count()
              << "  pending requests: " << m_cache->pending_request_count()
              << "  pending uploads: " << m_cache->pending_upload_count()
              << std::endl;
  }
  if (m_overview) {
    char const* ov = "idle";
    switch (m_overview->state()) {
      case ImageOverview::State::Idle:    ov = "idle"; break;
      case ImageOverview::State::Loading: ov = "loading"; break;
      case ImageOverview::State::Ready:   ov = "ready"; break;
      case ImageOverview::State::Failed:  ov = "failed"; break;
    }
    std::cout << "    Overview: " << ov << std::endl;
  }
}

int
Image::pending_tile_requests() const
{
  return m_cache ? m_cache->pending_request_count() : 0;
}

int
Image::pending_tile_uploads() const
{
  return m_cache ? m_cache->pending_upload_count() : 0;
}

int
Image::tile_cache_entries() const
{
  return m_cache ? m_cache->cache_entry_count() : 0;
}

int
Image::ready_tile_surfaces() const
{
  return m_cache ? m_cache->ready_surface_count() : 0;
}

URL
Image::get_url() const
{
  return m_url;
}

void
Image::draw_mark(wstdisplay::GraphicsContext& gc)
{
  gc.draw_rect(get_image_rect(), surf::Color::from_rgb888(255, 255, 255));
}

void
Image::on_leave_screen()
{
  WorkspaceItem::on_leave_screen();
  cache_cleanup();
  // Keep overview if already Ready; abort in-flight to save work off-screen.
  if (m_overview && m_overview->state() == ImageOverview::State::Loading) {
    m_overview->clear();
  }
}

} // namespace galapix

/* EOF */
