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

#ifndef HEADER_GALAPIX_UTIL_SOFTWARE_SURFACE_HPP
#define HEADER_GALAPIX_UTIL_SOFTWARE_SURFACE_HPP

#include <stdint.h>
#include <memory>

#include "math/vector2i.hpp"
#include "util/blob.hpp"
#include "util/pixel_data.hpp"

class RGB;
class RGBA;
class Rect;
class Size;

/** A non-mutable high-level SoftwareSurface with value semantics */
class SoftwareSurface
{
public:
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

public:
  SoftwareSurface();
  SoftwareSurface(PixelData data);

  Size get_size() const;
  int get_width() const;
  int get_height() const;

  SoftwareSurface halve() const;
  SoftwareSurface scale(Size const& size) const;
  SoftwareSurface crop(Rect const& rect) const;

  SoftwareSurface transform(Modifier mod) const;
  SoftwareSurface rotate90() const;
  SoftwareSurface rotate180() const;
  SoftwareSurface rotate270() const;
  SoftwareSurface vflip() const;
  SoftwareSurface hflip() const;
  SoftwareSurface to_rgb() const;
  RGB get_average_color() const;

  PixelData const& get_pixel_data() const { return *m_pixel_data; }

private:
  std::shared_ptr<const PixelData> m_pixel_data;
};

#endif

/* EOF */
