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

#ifndef HEADER_GALAPIX_UTIL_SOFTWARE_SURFACE_HPP
#define HEADER_GALAPIX_UTIL_SOFTWARE_SURFACE_HPP

#include <stdint.h>

#include "util/blob.hpp"

class Vector2i;
class RGB;
class RGBA;
class Rect;
class Size;
class SoftwareSurfaceImpl;
class SoftwareSurface;

typedef boost::shared_ptr<SoftwareSurface> SoftwareSurfaceHandle;

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
    kRot180,
    kRot270,
    kRot90,
    kRot0Flip,
    kRot180Flip,
    kRot270Flip,
    kRot90Flip
  };

private:
  SoftwareSurface(Format format, const Size& size);

public:
  static SoftwareSurfaceHandle create(Format format, const Size& size);

  Size get_size()   const;
  int  get_width()  const;
  int  get_height() const;
  int  get_pitch()  const;

  SoftwareSurfaceHandle clone();
  SoftwareSurfaceHandle halve();
  SoftwareSurfaceHandle scale(const Size& size);
  SoftwareSurfaceHandle crop(const Rect& rect);

  SoftwareSurfaceHandle transform(Modifier mod);
  SoftwareSurfaceHandle rotate90();
  SoftwareSurfaceHandle rotate180();
  SoftwareSurfaceHandle rotate270();
  SoftwareSurfaceHandle vflip();
  SoftwareSurfaceHandle hflip();

  BlobHandle get_raw_data()  const;
   
  void put_pixel(int x, int y, const RGB& rgb);
  void get_pixel(int x, int y, RGB& rgb) const;

  void put_pixel(int x, int y, const RGBA& rgb);
  void get_pixel(int x, int y, RGBA& rgb) const;

  uint8_t* get_data() const;
  uint8_t* get_row_data(int y) const;

  RGB get_average_color() const;

  Format get_format() const;

  SoftwareSurfaceHandle to_rgb();

  int get_bytes_per_pixel() const;

  /** Performs a simple copy from this to \a test, no blending is performed */
  void blit(SoftwareSurfaceHandle& dst, const Vector2i& pos);

private:
  boost::scoped_ptr<SoftwareSurfaceImpl> impl;
};

#endif

/* EOF */
