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

#include "math/rgb.hpp"
#include "display/framebuffer.hpp"
#include "database/file_entry.hpp"
#include "galapix/database_thread.hpp"
#include "galapix/viewer.hpp"
#include "galapix/image_tile_cache.hpp"
#include "galapix/image_renderer.hpp"

ImageHandle
Image::create(const FileEntry& file_entry)
{
  return ImageHandle(new Image(file_entry));
}

Image::Image(const FileEntry& file_entry) :
  m_file_entry(file_entry),
  m_max_scale(m_file_entry.get_thumbnail_scale()),
  m_pos(),
  m_last_pos(),
  m_target_pos(),
  m_scale(1.0f),
  m_last_scale(1.0f),
  m_target_scale(1.0f),
  m_angle(0.0f),
  m_cache(new ImageTileCache(m_file_entry)),
  m_renderer(new ImageRenderer(*this, *m_cache))
{
  assert(m_max_scale >= 0);
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
}

float
Image::get_scale() const
{
  return m_scale;
}

float
Image::get_scaled_width() const
{
  return static_cast<float>(m_file_entry.get_width()) * m_scale;
}

float
Image::get_scaled_height() const
{
  return static_cast<float>(m_file_entry.get_height()) * m_scale;
}

int
Image::get_original_width() const
{
  return m_file_entry.get_width();
}

int
Image::get_original_height() const
{
  return m_file_entry.get_height();
}

void
Image::clear_cache()
{
  m_cache->clear();
}

void
Image::cache_cleanup()
{
  m_cache->cleanup();
}

bool
Image::draw(const Rectf& cliprect, float zoom)
{
  if (m_file_entry)
  {
    m_cache->process_queue();
    return m_renderer->draw(cliprect, zoom);
  }
  else
  {
    return false;
  }
}

void
Image::refresh(bool force)
{
  if (force || m_file_entry.get_url().get_mtime() != m_file_entry.get_mtime())
    {
      clear_cache();
      DatabaseThread::current()->delete_file_entry(m_file_entry.get_fileid());

      // FIXME: Add this point the FileEntry is invalid and points to
      // something that no longer exists, newly generated Tiles point
      // to that obsolete fileid number
      // do stuff with receive_file_entry() to fix this
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
  return get_image_rect().contains(pos);
}

bool
Image::overlaps(const Rectf& cliprect) const
{
  return cliprect.is_overlapped(get_image_rect());
}

URL
Image::get_url() const
{
  return m_file_entry.get_url();
}

void
Image::draw_mark()
{
  Framebuffer::draw_rect(get_image_rect(), RGB(255, 255, 255));
}

Rectf
Image::get_image_rect() const
{
  if (!m_file_entry)
    {
      return Rectf(m_pos, Size(0, 0));
    }
  else
    {
      if (m_file_entry.get_image_size() == Size(0,0))
        {
          return Rectf(m_pos, m_file_entry.get_image_size());
        }
      else
        {
          Sizef image_size(m_file_entry.get_image_size() * m_scale);
          return Rectf(m_pos - Vector2f(image_size.width/2, image_size.height/2), image_size); // in world coordinates
        }
    }
}

/*
void
Image::receive_file_entry(const FileEntry& file_entry)
{
  assert(!m_file_entry);

  m_file_entry = file_entry;

  m_max_scale      = m_file_entry.get_thumbnail_scale();
}
*/

/* EOF */