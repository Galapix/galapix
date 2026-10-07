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

#ifndef HEADER_SURF_CONVERT_HPP
#define HEADER_SURF_CONVERT_HPP

#include "color.hpp"
#include "pixel.hpp"
#include "promote.hpp"

namespace surf {

template<typename SrcPixel, typename DstPixel> inline
constexpr typename DstPixel::value_type convert_value(typename SrcPixel::value_type v)
{
  using srctype = typename SrcPixel::value_type;
  using dsttype = typename DstPixel::value_type;

  if constexpr (std::is_same<SrcPixel, DstPixel>::value) {
    return v;
  } else {
    if constexpr (std::is_floating_point<srctype>::value) {
      if constexpr (std::is_floating_point<dsttype>::value) {
        // float -> float
        return static_cast<dsttype>(v);
      } else {
        // float -> int
        if constexpr (sizeof(dsttype) == sizeof(srctype)) {
          // special case, as uint32 -> float32 will overflow
          return static_cast<dsttype>(
            std::min(static_cast<uint64_t>(std::clamp<srctype>(v, 0.0, 1.0) * static_cast<srctype>(DstPixel::max()) + static_cast<srctype>(0.5)),
                     static_cast<uint64_t>(DstPixel::max())));
        } else {
          return static_cast<dsttype>(std::clamp<srctype>(v, 0.0, 1.0) * static_cast<srctype>(DstPixel::max()) + static_cast<srctype>(0.5));
        }
      }
    } else {
      if constexpr (std::is_floating_point<dsttype>::value) {
        // int -> float
        return static_cast<dsttype>(v) / static_cast<dsttype>(SrcPixel::max());
      } else {
        // int -> int
        using promotype = typename promote_t<typename SrcPixel::value_type, typename DstPixel::value_type>::type;
        return static_cast<dsttype>(static_cast<promotype>(v) * DstPixel::max() / SrcPixel::max());
      }
    }
  }
}

template<typename SrcPixel, typename DstPixel>
DstPixel convert(SrcPixel src)
{
  if constexpr (std::is_same<SrcPixel, DstPixel>::value) {
    return src;
  } else if constexpr (SrcPixel::has_alpha()) {
    return make_pixel<DstPixel>(
      convert_value<SrcPixel, DstPixel>(red(src)),
      convert_value<SrcPixel, DstPixel>(green(src)),
      convert_value<SrcPixel, DstPixel>(blue(src)),
      convert_value<SrcPixel, DstPixel>(alpha(src)));
  } else {
    return make_pixel<DstPixel>(
      convert_value<SrcPixel, DstPixel>(red(src)),
      convert_value<SrcPixel, DstPixel>(green(src)),
      convert_value<SrcPixel, DstPixel>(blue(src)),
      DstPixel::max());
  }
}

template<> inline
L8Pixel convert<RGB8Pixel, L8Pixel>(RGB8Pixel src) {
  // FIXME: add proper weighting of colors here
  return {static_cast<uint8_t>((red(src) + green(src) + blue(src)) / 3)};
}

template<> inline
RGB8Pixel convert<L8Pixel, RGB8Pixel>(L8Pixel src) {
  // FIXME: add proper weighting of colors here
  return {src.l, src.l, src.l};
}

template<> inline
RGBA8Pixel convert<L8Pixel, RGBA8Pixel>(L8Pixel src) {
  // FIXME: add proper weighting of colors here
  return {src.l, src.l, src.l, RGBA8Pixel::max()};
}

} // namespace surf

#endif

/* EOF */
