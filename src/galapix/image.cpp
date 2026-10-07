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

#include <algorithm>
#include <iostream>

#include <surf/color.hpp>
#include <wstdisplay/canvas.hpp>

namespace galapix {

Image::Image(URL const& url, TileProviderPtr provider) :
  m_url(url),
  m_provider(),
  m_tiles()
{
  set_tile_provider(std::move(provider));
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
  if (m_tiles) {
    m_tiles->clear();
  }
}

void
Image::cache_cleanup()
{
  // Off-screen images give their GPU textures back; the pixels stay with
  // the tile loader until its byte budget evicts them.
  if (m_tiles && !is_visible()) {
    m_tiles->release_textures();
  }
}

void
Image::prepare_tiles(wstdisplay::Device& device, Rectf const& cliprect, float zoom)
{
  if (m_tiles) {
    m_tiles->update(device, get_image_rect(), get_scale(), cliprect, zoom);
  }
}

void
Image::draw(wstdisplay::Canvas& canvas, Rectf const& cliprect, float zoom)
{
  if (!m_tiles)
  {
    canvas.fill_rect(get_image_rect(), surf::Color::from_rgb888(255,255,0));
  }
  else
  {
    m_tiles->draw(canvas, get_image_rect(), get_scale(), zoom);
  }
}

void
Image::set_tile_provider(TileProviderPtr provider)
{
  float old_size = static_cast<float>(std::max(get_original_width(),
                                               get_original_height()));

  m_tiles.reset();
  m_provider = std::move(provider);
  if (m_provider)
  {
    m_tiles = std::make_unique<ImageTiles>(m_provider);
  }

  // Fixup the scale to fit into the old constrains more or less (only
  // works with regular layouts)
  float new_size = static_cast<float>(std::max(get_original_width(),
                                               get_original_height()));
  set_scale(get_scale() * old_size / new_size);
}

ImageTiles::Stats
Image::tile_stats() const
{
  return m_tiles ? m_tiles->stats() : ImageTiles::Stats();
}

std::string
Image::tile_status_line() const
{
  return m_tiles ? m_tiles->status_line() : std::string("no size yet");
}

void
Image::print_info() const
{
  std::cout << "  Image: " << this << " " << m_url << std::endl;
  std::cout << "    " << tile_status_line() << std::endl;
}

URL
Image::get_url() const
{
  return m_url;
}

void
Image::draw_mark(wstdisplay::Canvas& canvas, float zoom)
{
  canvas.draw_rect(get_image_rect(), surf::Color::from_rgb888(255, 255, 255), 1.0f / zoom);
}

void
Image::on_leave_screen()
{
  WorkspaceItem::on_leave_screen();
  if (m_tiles) {
    m_tiles->hide();
  }
}

} // namespace galapix

/* EOF */
