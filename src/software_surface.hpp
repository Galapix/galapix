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

#ifndef HEADER_SOFTWARE_SURFACE_HPP
#define HEADER_SOFTWARE_SURFACE_HPP

#include <boost/shared_ptr.hpp>
#include "blob.hpp"

class RGB;
class Rect;
class Size;
class SoftwareSurfaceImpl;

class SoftwareSurface
{
public:
  enum Format { RGB_FORMAT, RGBA_FORMAT };

  SoftwareSurface();
  SoftwareSurface(const Size& size);

  ~SoftwareSurface();

  Size get_size()   const;
  int  get_width()  const;
  int  get_height() const;
  int  get_pitch()  const;

  SoftwareSurface halve() const;
  SoftwareSurface scale(const Size& size) const;
  SoftwareSurface crop(const Rect& rect) const;

  void save(const std::string& filename) const;
  
  Blob get_jpeg_data() const;
  Blob get_raw_data()  const;
  
  static SoftwareSurface from_jpeg_data(const Blob& blob);
  static SoftwareSurface from_raw_data(const Blob& blob);
  static SoftwareSurface from_file(const std::string& filename);
 
  void put_pixel(int x, int y, const RGB& rgb);
  void get_pixel(int x, int y, RGB& rgb) const;

  uint8_t* get_data() const;
  uint8_t* get_row_data(int y) const;

  RGB get_average_color() const;

  Format get_format() const;

  operator bool() const { return impl.get(); }

private:
  boost::shared_ptr<SoftwareSurfaceImpl> impl;
};

#endif

/* EOF */
