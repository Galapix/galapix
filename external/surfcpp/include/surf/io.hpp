// surf - Software surface library
// Copyright (C) 2008-2020 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at your
// option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
// License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_SURF_IO_HPP
#define HEADER_SURF_IO_HPP

#include <cstdint>
#include <format>
#include <ostream>

#include "pixel.hpp"
#include "pixel_view.hpp"
#include "software_surface.hpp"
#include "unwrap.hpp"

namespace surf {

inline
std::ostream& operator<<(std::ostream& os, RGB8Pixel const& pixel)
{
  return os << std::format("({:02x} {:02x} {:02x})",
                           static_cast<uint32_t>(pixel.r),
                           static_cast<uint32_t>(pixel.g),
                           static_cast<uint32_t>(pixel.b));
}

inline
std::ostream& operator<<(std::ostream& os, RGBA8Pixel const& pixel)
{
  return os << std::format("({:02x} {:02x} {:02x} {:02x})",
                           static_cast<uint32_t>(pixel.r),
                           static_cast<uint32_t>(pixel.g),
                           static_cast<uint32_t>(pixel.b),
                           static_cast<uint32_t>(pixel.a));
}

inline
std::ostream& operator<<(std::ostream& os, RGB16Pixel const& pixel)
{
  return os << std::format("({:04x} {:04x} {:04x})",
                           static_cast<uint32_t>(pixel.r),
                           static_cast<uint32_t>(pixel.g),
                           static_cast<uint32_t>(pixel.b));
}

inline
std::ostream& operator<<(std::ostream& os, RGBA16Pixel const& pixel)
{
  return os << std::format("({:04x} {:04x} {:04x} {:04x})",
                           static_cast<uint32_t>(pixel.r),
                           static_cast<uint32_t>(pixel.g),
                           static_cast<uint32_t>(pixel.b),
                           static_cast<uint32_t>(pixel.a));
}

inline
std::ostream& operator<<(std::ostream& os, RGB32Pixel const& pixel)
{
  return os << std::format("({:08x} {:08x} {:08x})",
                           static_cast<uint32_t>(pixel.r),
                           static_cast<uint32_t>(pixel.g),
                           static_cast<uint32_t>(pixel.b));
}

inline
std::ostream& operator<<(std::ostream& os, RGBA32Pixel const& pixel)
{
  return os << std::format("({:08x} {:08x} {:08x} {:08x})",
                           static_cast<uint32_t>(pixel.r),
                           static_cast<uint32_t>(pixel.g),
                           static_cast<uint32_t>(pixel.b),
                           static_cast<uint32_t>(pixel.a));
}

inline
std::ostream& operator<<(std::ostream& os, RGB32fPixel const& pixel)
{
  return os << std::format("({:.2f} {:.2f} {:.2f})",
                           pixel.r,
                           pixel.g,
                           pixel.b);
}

inline
std::ostream& operator<<(std::ostream& os, RGBA32fPixel const& pixel)
{
  return os << std::format("({:.2f} {:.2f} {:.2f} {:.2f})",
                           pixel.r,
                           pixel.g,
                           pixel.b,
                           pixel.a);
}
inline
std::ostream& operator<<(std::ostream& os, RGB64fPixel const& pixel)
{
  return os << std::format("({:.2f} {:.2f} {:.2f})",
                           pixel.r,
                           pixel.g,
                           pixel.b);
}

inline
std::ostream& operator<<(std::ostream& os, RGBA64fPixel const& pixel)
{
  return os << std::format("({:.2f} {:.2f} {:.2f} {:.2f})",
                           pixel.r,
                           pixel.g,
                           pixel.b,
                           pixel.a);
}

inline
std::ostream& operator<<(std::ostream& os, L8Pixel const& pixel)
{
  return os << std::format("({:02x})",
                           static_cast<uint32_t>(pixel.l));
}

inline
std::ostream& operator<<(std::ostream& os, LA8Pixel const& pixel)
{
  return os << std::format("({:02x} {:02x})",
                           static_cast<uint32_t>(pixel.l),
                           static_cast<uint32_t>(pixel.a));
}

inline
std::ostream& operator<<(std::ostream& os, L16Pixel const& pixel)
{
  return os << std::format("({:04x})",
                           static_cast<uint32_t>(pixel.l));
}

inline
std::ostream& operator<<(std::ostream& os, LA16Pixel const& pixel)
{
  return os << std::format("({:04x} {:04x})",
                           static_cast<uint32_t>(pixel.l),
                           static_cast<uint32_t>(pixel.a));
}

inline
std::ostream& operator<<(std::ostream& os, L32Pixel const& pixel)
{
  return os << std::format("({:08x})",
                           static_cast<uint32_t>(pixel.l));
}

inline
std::ostream& operator<<(std::ostream& os, LA32Pixel const& pixel)
{
  return os << std::format("({:08x} {:08x})",
                           static_cast<uint32_t>(pixel.l),
                           static_cast<uint32_t>(pixel.a));
}

inline
std::ostream& operator<<(std::ostream& os, L32fPixel const& pixel)
{
  return os << std::format("({:.2f})",
                           pixel.l);
}

inline
std::ostream& operator<<(std::ostream& os, LA32fPixel const& pixel)
{
  return os << std::format("({:.2f} {:.2f})",
                           pixel.l,
                           pixel.a);
}
inline
std::ostream& operator<<(std::ostream& os, L64fPixel const& pixel)
{
  return os << std::format("({:.2f})",
                           pixel.l);
}

inline
std::ostream& operator<<(std::ostream& os, LA64fPixel const& pixel)
{
  return os << std::format("({:.2f} {:.2f})",
                           pixel.l,
                           pixel.a);
}

inline
std::ostream& operator<<(std::ostream& os, Color const& color)
{
  return os << std::format("({:.2f} {:.2f} {:.2f} {:.2f})",
                           color.r,
                           color.g,
                           color.b,
                           color.a);
}

template<typename Pixel>
inline
std::ostream& operator<<(std::ostream& os, PixelView<Pixel> const& pixeldata)
{
  os << "\n{";
  for (int y = 0; y < pixeldata.get_height(); ++y) {
    if (y != 0) {
      os << " ";
    }
    os << " { ";
    for (int x = 0; x < pixeldata.get_width(); ++x) {
      Pixel pixel = pixeldata.get_pixel(geom::ipoint(x, y));
      os << pixel;
      if (x != pixeldata.get_width() - 1) {
        os << ' ';
      }
    }
    if (y == pixeldata.get_height() - 1) {
      os << " }";
    }
    os << " }\n";
  }
  return os;
}

inline
std::ostream& operator<<(std::ostream& os, SoftwareSurface const& surface)
{
  PIXELFORMAT_TO_TYPE(
    surface.get_format(), srctype,
    os << surface.as_pixelview<srctype>());
  return os;
}

} // namespace surf

#endif

/* EOF */
