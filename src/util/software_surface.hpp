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

#ifndef HEADER_GALAPIX_UTIL_SOFTWARE_SURFACE_HPP
#define HEADER_GALAPIX_UTIL_SOFTWARE_SURFACE_HPP

#include <stdint.h>
#include <memory>

#include "math/vector2i.hpp"
#include "util/blob.hpp"

class RGB;
class RGBA;
class Rect;
class Size;
class SoftwareSurfaceImpl;
class SoftwareSurface;

typedef std::shared_ptr<SoftwareSurface> SoftwareSurfacePtr;

class SoftwareSurface
{
public:
  enum Format
  {
    RGB_FORMAT,
    RGBA_FORMAT
  };

  enum Modifier
  {
    kRot0,
    kRot90,
    kRot180,
    kRot270,
    kRot0Flip,   // vflip
    kRot90Flip,
    kRot180Flip, // hflip
    kRot270Flip
  };

private:
  SoftwareSurface(Format format, const Size& size);

public:
  static SoftwareSurfacePtr create(Format format, const Size& size);

  Size get_size() const;
  int get_width() const;
  int get_height() const;
  int get_pitch() const;

  SoftwareSurfacePtr clone() const;
  SoftwareSurfacePtr halve() const;
  SoftwareSurfacePtr scale(const Size& size) const;
  SoftwareSurfacePtr crop(const Rect& rect) const;

  SoftwareSurfacePtr transform(Modifier mod) const;
  SoftwareSurfacePtr rotate90() const;
  SoftwareSurfacePtr rotate180() const;
  SoftwareSurfacePtr rotate270() const;
  SoftwareSurfacePtr vflip() const;
  SoftwareSurfacePtr hflip() const;

  void get_pixel(int x, int y, RGB& rgb) const;
  void get_pixel(int x, int y, RGBA& rgb) const;

  uint8_t const* get_data() const;
  uint8_t const* get_row_data(int y) const;

  RGB get_average_color() const;

  Format get_format() const;

  SoftwareSurfacePtr to_rgb() const;

  int get_bytes_per_pixel() const;

  /** Performs a simple copy from this to \a test, no blending is performed */
  void blit(SoftwareSurface& dst, const Vector2i& pos);

  void put_pixel(int x, int y, const RGB& rgb);
  void put_pixel(int x, int y, const RGBA& rgb);

  uint8_t* get_data();
  uint8_t* get_row_data(int y);

private:
  std::unique_ptr<SoftwareSurfaceImpl> impl;
};

#endif

/* EOF */
