// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/texture_packer.hpp>

#include <cstring>
#include <stdexcept>

#include <surf/palette.hpp>
#include <surf/save.hpp>

#include <wstdisplay/blitter.hpp>
#include <wstdisplay/surface.hpp>
#include <wstdisplay/texture.hpp>

namespace wstdisplay {

TexturePacker::TexturePacker(Device& device, geom::isize const& page_size, TextureParams const& params) :
  m_device(device),
  m_page_size(page_size),
  m_params(params),
  m_pages()
{
}

TexturePacker::~TexturePacker()
{
}

Surface
TexturePacker::upload(surf::SoftwareSurface const& image)
{
  // one pixel border on each side
  geom::isize const size(image.get_width() + 2, image.get_height() + 2);
  if (size.width() > m_page_size.width() || size.height() > m_page_size.height()) {
    throw std::invalid_argument("TexturePacker::upload: image larger than a page");
  }

  Page* page = nullptr;
  std::optional<geom::irect> rect;
  for (auto& p : m_pages) {
    if ((rect = p.packer.allocate(size))) {
      page = &p;
      break;
    }
  }

  if (!page) {
    m_pages.push_back(Page{m_device.create_texture(m_page_size, m_params), RectPacker(m_page_size)});
    page = &m_pages.back();
    rect = page->packer.allocate(size);
  }

  surf::SoftwareSurface bordered = surf::SoftwareSurface::create(surf::PixelFormat::RGBA8, size,
                                                                 surf::palette::transparent);
  // copy row by row, image might be a view into a larger surface
  surf::SoftwareSurface converted;
  surf::SoftwareSurface const* src = &image;
  if (image.get_format() != surf::PixelFormat::RGBA8) {
    converted = surf::convert(image, surf::PixelFormat::RGBA8);
    src = &converted;
  }
  size_t const row_size = static_cast<size_t>(image.get_width()) * 4;
  for (int y = 0; y < image.get_height(); ++y) {
    std::memcpy(static_cast<uint8_t*>(bordered.get_row_data(y + 1)) + 4, src->get_row_data(y), row_size);
  }
  generate_border(bordered, 1, 1, image.get_width(), image.get_height());

  page->texture->put(bordered, rect->topleft());

  float const pw = static_cast<float>(m_page_size.width());
  float const ph = static_cast<float>(m_page_size.height());
  geom::frect const uv(static_cast<float>(rect->left() + 1) / pw,
                       static_cast<float>(rect->top() + 1) / ph,
                       static_cast<float>(rect->right() - 1) / pw,
                       static_cast<float>(rect->bottom() - 1) / ph);

  return Surface(page->texture, uv, geom::fsize(image.get_size()));
}

std::vector<TextureId>
TexturePacker::get_pages() const
{
  std::vector<TextureId> pages;
  for (auto const& page : m_pages) {
    pages.push_back(page.texture);
  }
  return pages;
}

void
TexturePacker::save_all_as_png(std::filesystem::path const& directory) const
{
  for (size_t i = 0; i < m_pages.size(); ++i) {
    surf::save(m_pages[i].texture->get_software_surface(),
               directory / ("page-" + std::to_string(i) + ".png"));
  }
}

} // namespace wstdisplay

/* EOF */
