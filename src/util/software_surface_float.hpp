/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2009 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_UTIL_SOFTWARE_SURFACE_FLOAT_HPP
#define HEADER_GALAPIX_UTIL_SOFTWARE_SURFACE_FLOAT_HPP

#include <boost/shared_ptr.hpp>
#include <vector>

#include "math/size.hpp"
#include "math/rgbaf.hpp"
#include "util/software_surface.hpp"

class SoftwareSurfaceFloat;

typedef boost::shared_ptr<SoftwareSurfaceFloat> SoftwareSurfaceFloatPtr;

class SoftwareSurfaceFloat
{
public:
  static SoftwareSurfaceFloatPtr create(const Size& size);
  static SoftwareSurfaceFloatPtr from_software_surface(SoftwareSurface const& surface);

private:
  SoftwareSurfaceFloat(const Size& size);

public:
  SoftwareSurface to_software_surface() const;

  void apply_gamma(float gamma);

  Size get_size()   const;
  int  get_width()  const;
  int  get_height() const;
  int  get_pitch()  const;

  void put_pixel(int x, int y, const RGBAf& rgba);
  void get_pixel(int x, int y, RGBAf& rgba) const;

private:
  Size m_size;
  std::vector<float> m_pixels;
};

#endif

/* EOF */
