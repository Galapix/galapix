// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_TEXTURE_PACKER_HPP
#define HEADER_WSTDISPLAY_TEXTURE_PACKER_HPP

#include <filesystem>
#include <vector>

#include <geom/size.hpp>
#include <surf/software_surface.hpp>

#include "device.hpp"
#include "fwd.hpp"
#include "rect_packer.hpp"
#include "surface.hpp"

namespace wstdisplay {

/** Packs images onto shared texture pages (an atlas), so that
    drawing different images doesn't require switching textures. Each
    image gets a one pixel border of duplicated edge pixels to avoid
    bleeding with linear filtering. The packer owns the pages. */
class TexturePacker final
{
public:
  TexturePacker(Device& device, geom::isize const& page_size, TextureParams const& params = {});
  ~TexturePacker();

  /** Upload \a image onto a page, a new page is added when no page
      has room. Throws if the image is larger than a page. */
  Surface upload(surf::SoftwareSurface const& image);

  geom::isize get_page_size() const { return m_page_size; }
  std::vector<TextureId> get_pages() const;

  /** Save every page as page-N.png into \a directory, for debugging */
  void save_all_as_png(std::filesystem::path const& directory) const;

private:
  struct Page
  {
    Unique<Texture> texture = {};
    RectPacker packer;
  };

  Device& m_device;
  geom::isize m_page_size;
  TextureParams m_params;
  std::vector<Page> m_pages;

public:
  TexturePacker(TexturePacker const&) = delete;
  TexturePacker& operator=(TexturePacker const&) = delete;
};

} // namespace wstdisplay

#endif

/* EOF */
