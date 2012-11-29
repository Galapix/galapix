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

#include "galapix/image.hpp"

#include <functional>
#include <iostream>

#include "database/entries/file_entry.hpp"
#include "display/framebuffer.hpp"
#include "galapix/database_thread.hpp"
#include "galapix/database_tile_provider.hpp"
#include "galapix/image_renderer.hpp"
#include "galapix/image_tile_cache.hpp"
#include "galapix/viewer.hpp"
#include "math/rgb.hpp"
#include "util/weak_functor.hpp"

Image::Image(const URL& url, TileProviderPtr provider) :
  m_url(url),
  m_provider(provider),
  m_cache(),
  m_renderer()
{
  set_provider(m_provider);
}

Image::~Image()
{
}

int
Image::get_original_width() const
{
  if (m_provider)
  {
    return m_provider->get_size().width;
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
    return m_provider->get_size().height;
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
Image::draw(const Rectf& cliprect, float zoom)
{
  if (!m_provider)
  {
    Framebuffer::fill_rect(Rectf(get_top_left_pos(), Sizef(get_scaled_width(), get_scaled_height())),
                           RGB(255,255,0));
  }
  else
  {
    m_cache->process_queue();
    m_renderer->draw(cliprect, zoom);
  }
}

void
Image::set_provider(TileProviderPtr provider)
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
    m_provider = provider;
    m_cache    = std::make_shared<ImageTileCache>(m_provider);
    m_renderer.reset(new ImageRenderer(*this, m_cache));
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
Image::draw_mark()
{
  Framebuffer::draw_rect(get_image_rect(), RGB(255, 255, 255));
}

void
Image::on_leave_screen()
{
  WorkspaceItem::on_leave_screen();
  cache_cleanup();
}

/* EOF */
