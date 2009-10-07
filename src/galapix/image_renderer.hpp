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

#ifndef HEADER_GALAPIX_GALAPIX_IMAGE_RENDERER_HPP
#define HEADER_GALAPIX_GALAPIX_IMAGE_RENDERER_HPP

class Image;
class ImageTileCache;
class Rect;
class Rectf;
class Vector2f;

class ImageRenderer
{
private:
  ImageTileCache& m_cache;

public:
  ImageRenderer(ImageTileCache& cache);

  void draw(const Rectf& cliprect, float fscale, float impl_scale, Image& m_image);

private:
  void draw_tile(int x, int y, int tiledb_scale, const Vector2f& rect, float scale, Image& m_image);
  void draw_tiles(const Rect& rect, int tiledb_scale, const Vector2f& pos, float scale, Image& m_image);

private:
  ImageRenderer(const ImageRenderer&);
  ImageRenderer& operator=(const ImageRenderer&);
};

#endif

/* EOF */
