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

#include <boost/bind.hpp>
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
  m_image_rect(),
  m_pos(),
  m_last_pos(),
  m_target_pos(),
  m_scale(1.0f),
  m_last_scale(1.0f),
  m_target_scale(1.0f),
  m_angle(0.0f),
  m_file_entry_requested(false),
  m_cache(),
  m_renderer(),
  m_file_entry_queue(),
  m_tile_queue(),
  m_jobs()
{
  set_provider(m_provider);
}

Vector2f
Image::get_top_left_pos() const
{
  return m_pos - Vector2f(get_scaled_width()/2, get_scaled_height()/2);
}

void
Image::set_pos(const Vector2f& pos)
{
  m_pos        = pos;
  m_last_pos   = pos;
  m_target_pos = pos;

  m_image_rect = calc_image_rect();
}

Vector2f
Image::get_pos() const
{
  return m_pos;
}

void
Image::set_target_pos(const Vector2f& target_pos)
{
  m_last_pos   = m_pos;
  m_target_pos = target_pos;
}

void
Image::set_target_scale(float target_scale)
{
  m_last_scale   = m_scale;
  m_target_scale = target_scale;  
}

void
Image::update_pos(float progress)
{
  assert(progress >= 0.0f &&
         progress <= 1.0f);

  if (progress == 1.0f)
  {
    set_pos(m_target_pos);
    set_scale(m_target_scale);
  }
  else
  {
    m_pos   = (m_last_pos   * (1.0f - progress)) + (m_target_pos   * progress);
    m_scale = (m_last_scale * (1.0f - progress)) + (m_target_scale * progress);
    m_image_rect = calc_image_rect();
  }
}

void
Image::set_angle(float a)
{
  m_angle = a;
}

float
Image::get_angle() const
{
  return m_angle;
}

void
Image::set_scale(float f)
{
  m_scale        = f;
  m_last_scale   = f;
  m_target_scale = f;

  m_image_rect = calc_image_rect();
}

float
Image::get_scale() const
{
  return m_scale;
}

float
Image::get_scaled_width() const
{
  return static_cast<float>(get_original_width()) * m_scale;
}

float
Image::get_scaled_height() const
{
  return static_cast<float>(get_original_height()) * m_scale;
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
  m_file_entry_requested = false;
}

void
Image::abort_all_jobs()
{
  for(Jobs::iterator i = m_jobs.begin(); i != m_jobs.end(); ++i)
  {
    i->set_aborted();
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
  m_file_entry_requested = false;
}

void
Image::process_queues()
{
  // Check if there was an update of the FileEntry
  while(!m_file_entry_queue.empty())
  {
    FileEntry file_entry = m_file_entry_queue.front();
    m_file_entry_queue.pop();

    if (file_entry.get_image_size() == Size(0, 0))
    { // reject images with invalid size
      std::cout << "Image::Image(): invalid image size: " << file_entry << std::endl;
    }
    else
    {
      m_file_entry_requested = false;
      set_provider(DatabaseTileProvider::create(file_entry));
    }
  }

  while(!m_tile_queue.empty())
  {
    if (m_cache)
    {
      m_cache->receive_tile(m_tile_queue.front());
    }

    m_tile_queue.pop();
  }

  while(!m_tile_provider_queue.empty())
  {
    set_provider(m_tile_provider_queue.front());
    m_tile_provider_queue.pop();
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

    if (!m_file_entry_requested)
    {
      m_file_entry_requested = true;
      m_jobs.push_back(DatabaseThread::current()->request_file(m_url,
                                                               weak(boost::bind(&Image::receive_file_entry, _1, _2), m_self),
                                                               weak(boost::bind(&Image::receive_tile, _1, _2, _3), m_self)));
    }
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
  m_scale *= old_size / new_size;
  m_target_scale = m_scale;

  m_image_rect = calc_image_rect();
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
      m_provider->refresh(weak(boost::bind(&Image::receive_tile_provider, _1, _2), m_self));
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

bool
Image::overlaps(const Vector2f& pos) const
{
  return m_image_rect.contains(pos);
}

bool
Image::overlaps(const Rectf& cliprect) const
{
  return m_image_rect.is_overlapped(cliprect);
}

URL
Image::get_url() const
{
  return m_url;
}

void
Image::draw_mark()
{
  Framebuffer::draw_rect(m_image_rect, RGB(255, 255, 255));
}

Rectf
Image::get_image_rect() const
{
  return m_image_rect;
}

Rectf
Image::calc_image_rect() const
{
  return Rectf(get_top_left_pos(),
               Sizef(get_scaled_width(), 
                     get_scaled_height()));
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
Image::receive_file_entry(const FileEntry& file_entry)
{
  // std::cout << "Image::receive_file_entry: " << file_entry << std::endl;
  assert(file_entry);
  m_file_entry_queue.push(file_entry);
}

void

Image::receive_tile(const FileEntry& file_entry, const Tile& tile)
{
  m_tile_queue.push(tile);
}

void
Image::receive_tile_provider(TileProviderPtr provider)
{
  m_tile_provider_queue.push(provider);
}

/* EOF */
