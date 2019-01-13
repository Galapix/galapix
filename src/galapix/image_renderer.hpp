/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_GALAPIX_IMAGE_RENDERER_HPP
#define HEADER_GALAPIX_GALAPIX_IMAGE_RENDERER_HPP

#include <memory>

#include "math/vector2f.hpp"

class Image;
class ImageTileCache;
class Rect;
class Rectf;

class ImageRenderer
{
public:
  ImageRenderer(Image& image, std::shared_ptr<ImageTileCache> const& cache);

  /** Return true if something was drawn to the screen, false when
      image was outside the cliprect */
  bool draw(const Rectf& cliprect, float zoom);

private:
  Vector2f get_vertex(int x, int y, float zoom) const;
  void draw_tile(int x, int y, int tiledb_scale, float zoom);
  void draw_tiles(const Rect& rect, int tiledb_scale, float zoom);

private:
  Image& m_image;
  std::shared_ptr<ImageTileCache> m_cache;

private:
  ImageRenderer(const ImageRenderer&);
  ImageRenderer& operator=(const ImageRenderer&);
};

#endif

/* EOF */
