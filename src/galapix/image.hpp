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
#include <string>

#include "galapix/image_tiles.hpp"
#include "galapix/tile_provider.hpp"
#include "galapix/workspace_item.hpp"
#include "math/rect.hpp"
#include "math/vector2f.hpp"
#include "util/url.hpp"

namespace galapix {

class Image final : public WorkspaceItem
{
public:
  Image(URL const& url, TileProviderPtr provider = {});
  ~Image() override;

  /** Visible this frame: publish tile demand, upload textures. */
  void prepare_tiles(wstdisplay::Device& device, Rectf const& cliprect, float zoom) override;

  void draw(wstdisplay::Canvas& canvas, Rectf const& cliprect, float zoom) override;
  void draw_mark(wstdisplay::Canvas& canvas, float zoom) override;

  URL get_url() const override;

  int get_original_width() const override;
  int get_original_height() const override;

  void clear_cache() override;
  void cache_cleanup() override;

  void print_info() const override;

  void on_leave_screen() override;

  /** Tile state, empty while the image has no provider (size probe). */
  ImageTiles::Stats tile_stats() const;
  std::string tile_status_line() const;
  bool has_tiles() const { return static_cast<bool>(m_tiles); }

  /** Attach or replace the tile provider (e.g. after async size probe). */
  void set_tile_provider(TileProviderPtr provider);
  TileProviderPtr get_tile_provider() const { return m_provider; }

private:
  URL m_url;
  TileProviderPtr m_provider;
  std::unique_ptr<ImageTiles> m_tiles;

private:
  Image(Image const&) = delete;
  Image& operator=(Image const&) = delete;
};

} // namespace galapix

#endif

/* EOF */
