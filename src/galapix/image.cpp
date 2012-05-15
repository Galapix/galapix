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

#include "database/file_entry.hpp"
#include "display/framebuffer.hpp"
#include "galapix/database_thread.hpp"
#include "galapix/database_tile_provider.hpp"
#include "galapix/image_renderer.hpp"
#include "galapix/image_tile_cache.hpp"
#include "galapix/viewer.hpp"
#include "math/rgb.hpp"
#include "util/weak_functor.hpp"

ImagePtr
Image::create(const URL& url, TileProviderPtr provider)
{
  ImagePtr image(new Image(url, provider));
  image->set_weak_ptr(image);
  return image;
}

void
Image::set_weak_ptr(ImagePtr self)
{
  m_self = self;
}

Image::Image(const URL& url, TileProviderPtr provider) :
  m_self(),
  m_url(url),
  m_provider(provider),
  m_visible(false),
  m_cache(),
  m_renderer(),
  m_tile_provider_queue(),
  m_jobs()
{
  set_provider(m_provider);
}

Image::~Image()
{
  for(auto& job: m_jobs)
  {
    job.set_aborted();
  }
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
  abort_all_jobs();
}

void
Image::abort_all_jobs()
{
  for(auto& job: m_jobs)
  {
    job.set_aborted();
  }
  m_jobs.clear();  
}

void
Image::cache_cleanup()
{
  if (m_cache)
  {
    m_cache->cleanup();
  }
  abort_all_jobs();
}

void
Image::process_queues()
{
  TileProviderPtr tile_provider;
  while(m_tile_provider_queue.try_pop(tile_provider))
  {
    set_provider(tile_provider);
  }
}

void
Image::draw(const Rectf& cliprect, float zoom)
{
  process_queues();

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
    abort_all_jobs();

    m_cache.reset();
    m_renderer.reset();
    m_provider.reset();
  }

  // set the new provider and related data
  if (provider)
  {
    m_provider = provider;
    m_cache    = ImageTileCache::create(m_provider);
    m_renderer.reset(new ImageRenderer(*this, m_cache));
  }

  // Fixup the scale to fit into the old constrains more or less (only
  // works with regular layouts)
  float new_size = static_cast<float>(std::max(get_original_width(),
                                               get_original_height()));
  set_scale(get_scale() * old_size / new_size);
}

void
Image::clear_provider()
{
  set_provider(TileProviderPtr());
}

void
Image::refresh(bool force)
{
  if (force)
  {
    if (m_provider)
    {
      m_provider->refresh(weak(std::bind(&Image::receive_tile_provider, std::placeholders::_1, std::placeholders::_2), m_self));
      clear_provider();
    }
  }
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
Image::on_enter_screen()
{
  m_visible = true;
  //std::cout << "Image::on_enter_screen(): " << this << std::endl;
}

void
Image::on_leave_screen()
{
  m_visible = false;
  //std::cout << "Image::on_leave_screen(): " << this << std::endl;
  cache_cleanup();
}

void
Image::on_zoom_level_change()
{
  abort_all_jobs();
}

void
Image::receive_tile_provider(TileProviderPtr provider)
{
  m_tile_provider_queue.wait_and_push(provider);
}

/* EOF */
