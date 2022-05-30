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

#include "database/entries/old_file_entry.hpp"
#include "galapix/database_tile_provider.hpp"
#include "galapix/image_renderer.hpp"
#include "galapix/image_tile_cache.hpp"
#include "galapix/viewer.hpp"
#include "server/database_thread.hpp"
#include "util/weak_functor.hpp"

using namespace surf;

Image::Image(URL const& url, TileProviderPtr provider) :
  m_url(url),
  m_provider(std::move(provider)),
  m_cache(),
  m_renderer()
{
  set_tile_provider(m_provider);
}

Image::~Image()
{
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
Image::draw(wstdisplay::GraphicsContext& gc, Rectf const& cliprect, float zoom)
{
  if (!m_provider)
  {
    gc.fill_rect(Rectf(get_top_left_pos(), Sizef(get_scaled_width(), get_scaled_height())),
                 surf::Color::from_rgb888(255,255,0));
  }
  else
  {
    m_cache->process_queue();
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
  //std::cout << "    Cache Size: " << m_cache.size() << std::endl;
  //std::cout << "    Job Size:   " << m_jobs.size() << std::endl;
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
}

/* EOF */
