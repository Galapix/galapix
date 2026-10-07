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

#ifndef HEADER_SURF_BLEND_HPP
#define HEADER_SURF_BLEND_HPP

#include <algorithm>
#include <cstdint>

#include "color.hpp"
#include "convert.hpp"
#include "pixel.hpp"

namespace surf {

namespace detail {

/** Convert a value in the range [0, 1] to DstPixel::value_type */
template<typename DstPixel> inline
typename DstPixel::value_type from_unit(double v)
{
  using dsttype = typename DstPixel::value_type;

  if constexpr (std::is_floating_point<dsttype>::value) {
    return static_cast<dsttype>(v);
  } else {
    return static_cast<dsttype>(std::clamp(v, 0.0, 1.0) * static_cast<double>(DstPixel::max()) + 0.5);
  }
}

template<typename Pixel> inline
double to_unit(typename Pixel::value_type v)
{
  return static_cast<double>(v) / static_cast<double>(Pixel::max());
}

} // namespace detail

template<typename SrcPixel, typename DstPixel>
struct pixel_copy
{
  inline DstPixel operator()(SrcPixel src, DstPixel dst)
  {
    return convert<SrcPixel, DstPixel>(src);
  }
};

template<typename SrcPixel, typename DstPixel>
struct pixel_blend
{
  inline DstPixel operator()(SrcPixel src, DstPixel dst)
  {
    using srctype = typename SrcPixel::value_type;
    using dsttype = typename DstPixel::value_type;

    if constexpr (!SrcPixel::has_alpha()) {
      return convert<SrcPixel, DstPixel>(src);
    } else if constexpr (std::is_floating_point<srctype>::value ||
                         std::is_floating_point<dsttype>::value ||
                         sizeof(dsttype) >= 4) {
      // the integer path below would overflow for 32bit values
      double const sa = detail::to_unit<SrcPixel>(alpha(src));
      double const sr = detail::to_unit<SrcPixel>(red(src));
      double const sg = detail::to_unit<SrcPixel>(green(src));
      double const sb = detail::to_unit<SrcPixel>(blue(src));

      double const dr = detail::to_unit<DstPixel>(red(dst));
      double const dg = detail::to_unit<DstPixel>(green(dst));
      double const db = detail::to_unit<DstPixel>(blue(dst));

      if constexpr (!DstPixel::has_alpha()) {
        return make_pixel<DstPixel>(
          detail::from_unit<DstPixel>(sr * sa + dr * (1.0 - sa)),
          detail::from_unit<DstPixel>(sg * sa + dg * (1.0 - sa)),
          detail::from_unit<DstPixel>(sb * sa + db * (1.0 - sa)));
      } else {
        double const da = detail::to_unit<DstPixel>(alpha(dst));
        double const out_a = sa + da * (1.0 - sa);
        if (out_a == 0.0) {
          return make_pixel<DstPixel>(0, 0, 0, 0);
        } else {
          return make_pixel<DstPixel>(
            detail::from_unit<DstPixel>((sr * sa + dr * da * (1.0 - sa)) / out_a),
            detail::from_unit<DstPixel>((sg * sa + dg * da * (1.0 - sa)) / out_a),
            detail::from_unit<DstPixel>((sb * sa + db * da * (1.0 - sa)) / out_a),
            detail::from_unit<DstPixel>(out_a));
        }
      }
    } else {
      // integer path, all values are converted to the DstPixel range
      using acc = uint64_t;
      acc const max = DstPixel::max();

      acc const sa = convert_value<SrcPixel, DstPixel>(alpha(src));
      acc const sr = convert_value<SrcPixel, DstPixel>(red(src));
      acc const sg = convert_value<SrcPixel, DstPixel>(green(src));
      acc const sb = convert_value<SrcPixel, DstPixel>(blue(src));

      if constexpr (!DstPixel::has_alpha()) {
        return make_pixel<DstPixel>(
          static_cast<dsttype>((sr * sa + red(dst) * (max - sa)) / max),
          static_cast<dsttype>((sg * sa + green(dst) * (max - sa)) / max),
          static_cast<dsttype>((sb * sa + blue(dst) * (max - sa)) / max));
      } else {
        acc const da = alpha(dst);
        acc const out_a = sa + da * (max - sa) / max;
        if (out_a == 0) {
          return make_pixel<DstPixel>(0, 0, 0, 0);
        } else {
          return make_pixel<DstPixel>(
            static_cast<dsttype>((sr * sa + red(dst) * da / max * (max - sa)) / out_a),
            static_cast<dsttype>((sg * sa + green(dst) * da / max * (max - sa)) / out_a),
            static_cast<dsttype>((sb * sa + blue(dst) * da / max * (max - sa)) / out_a),
            static_cast<dsttype>(out_a));
        }
      }
    }
  }
};

template<typename SrcPixel, typename DstPixel>
struct pixel_add
{
  inline constexpr DstPixel operator()(SrcPixel const src, DstPixel const dst)
  {
    using dsttype = typename DstPixel::value_type;

    if constexpr (SrcPixel::has_alpha()) {
      if (alpha(src) == 0) {
        return dst;
      }
    }

    if constexpr (std::is_floating_point<dsttype>::value) {
      return make_pixel<DstPixel>(
        static_cast<dsttype>(red_f(dst) + red_f(src) * alpha_f(src)),
        static_cast<dsttype>(green_f(dst) + green_f(src) * alpha_f(src)),
        static_cast<dsttype>(blue_f(dst) + blue_f(src) * alpha_f(src)),
        static_cast<dsttype>(alpha_f(dst))
        );
    } else {
      using acc = uint64_t;
      acc const max = DstPixel::max();

      acc const r = convert_value<SrcPixel, DstPixel>(red(src));
      acc const g = convert_value<SrcPixel, DstPixel>(green(src));
      acc const b = convert_value<SrcPixel, DstPixel>(blue(src));
      acc const a = convert_value<SrcPixel, DstPixel>(alpha(src));

      return make_pixel<DstPixel>(
        clamp_pixel_max<DstPixel>(red(dst) + r * a / max),
        clamp_pixel_max<DstPixel>(green(dst) + g * a / max),
        clamp_pixel_max<DstPixel>(blue(dst) + b * a / max),
        alpha(dst));
    }
  }
};

template<typename SrcPixel, typename DstPixel>
struct pixel_multiply
{
  inline constexpr DstPixel operator()(SrcPixel const src, DstPixel const dst)
  {
    using dsttype = typename DstPixel::value_type;

    if constexpr (SrcPixel::has_alpha()) {
      if (alpha(src) == 0) {
        return dst;
      }
    }

    // FIXME: ignoring src alpha for now
    if constexpr (std::is_floating_point<dsttype>::value) {
      return make_pixel<DstPixel>(
        static_cast<dsttype>(red_f(dst) * red_f(src)),
        static_cast<dsttype>(green_f(dst) * green_f(src)),
        static_cast<dsttype>(blue_f(dst) * blue_f(src)),
        static_cast<dsttype>(alpha_f(dst))
        );
    } else {
      using acc = uint64_t;
      acc const max = DstPixel::max();

      acc const r = convert_value<SrcPixel, DstPixel>(red(src));
      acc const g = convert_value<SrcPixel, DstPixel>(green(src));
      acc const b = convert_value<SrcPixel, DstPixel>(blue(src));

      return make_pixel<DstPixel>(
        clamp_pixel_max<DstPixel>(red(dst) * r / max),
        clamp_pixel_max<DstPixel>(green(dst) * g / max),
        clamp_pixel_max<DstPixel>(blue(dst) * b / max),
        alpha(dst));
    }
  }
};

} // namespace surf

#endif

/* EOF */
