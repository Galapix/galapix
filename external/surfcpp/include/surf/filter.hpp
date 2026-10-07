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

#ifndef HEADER_SURF_FILTER_HPP
#define HEADER_SURF_FILTER_HPP

#include <cmath>
#include <numbers>
#include <stdexcept>

#include "algorithm.hpp"
#include "blit.hpp"
#include "color.hpp"
#include "hsv.hpp"
#include "pixel_view.hpp"
#include "unwrap.hpp"

namespace surf {

template<typename Pixel, typename PixelFunc>
void pixel_filter(PixelView<Pixel>& src, PixelFunc func)
{
  for(int y = 0; y < src.get_height(); ++y) {
    Pixel* const row = src.get_row(y);
    for(int x = 0; x < src.get_width(); ++x) {
      row[x] = func(row[x]);
    }
  }
}

template<typename Pixel>
void apply_gamma(PixelView<Pixel>& src, float gamma)
{
  // FIXME: Slow
  for(int y = 0; y < src.get_height(); ++y) {
    for(int x = 0; x < src.get_width(); ++x) {
      Color rgba = src.get_pixel_color({x, y});
      rgba.r = powf(rgba.r, 1.0f / gamma);
      rgba.g = powf(rgba.g, 1.0f / gamma);
      rgba.b = powf(rgba.b, 1.0f / gamma);
      src.put_pixel_color({x, y}, rgba);
    }
  }
}

template<typename Pixel>
void apply_multiply(PixelView<Pixel>& src, float factor)
{
  // FIXME: Slow
  for(int y = 0; y < src.get_height(); ++y) {
    for(int x = 0; x < src.get_width(); ++x) {
      Color rgba = src.get_pixel_color({x, y});
      rgba.r *= factor;
      rgba.g *= factor;
      rgba.b *= factor;
      src.put_pixel_color({x, y}, rgba);
    }
  }
}

template<typename Pixel>
void apply_add(PixelView<Pixel>& src, float addend)
{
  using type = typename Pixel::value_type;

  if constexpr (Pixel::is_floating_point()) {
    type const addend_v = static_cast<type>(addend);
    for_each_pixel(src, [addend_v](Pixel& pixel) {
      pixel = make_pixel<Pixel>(
        red(pixel) + addend_v,
        green(pixel) + addend_v,
        blue(pixel) + addend_v,
        alpha(pixel));
    });
  } else {
    // signed, so that a negative addend darkens the image
    int64_t const addend_v = std::llround(static_cast<double>(std::clamp(addend, -1.0f, 1.0f)) *
                                          static_cast<double>(Pixel::max()));
    for_each_pixel(src, [addend_v](Pixel& pixel) {
      pixel = make_pixel<Pixel>(
        clamp_pixel<Pixel>(static_cast<int64_t>(red(pixel)) + addend_v),
        clamp_pixel<Pixel>(static_cast<int64_t>(green(pixel)) + addend_v),
        clamp_pixel<Pixel>(static_cast<int64_t>(blue(pixel)) + addend_v),
        alpha(pixel));
    });
  }
}

template<typename Pixel>
void apply_brightness(PixelView<Pixel>& src, float brightness)
{
  // FIXME: Slow
  for(int y = 0; y < src.get_height(); ++y) {
    for(int x = 0; x < src.get_width(); ++x) {
      Color rgba = src.get_pixel_color({x, y});
      rgba.r += brightness;
      rgba.g += brightness;
      rgba.b += brightness;
      src.put_pixel_color({x, y}, rgba);
    }
  }
}

template<typename Pixel>
void apply_contrast(PixelView<Pixel>& src, float contrast /* [-1.0, 1.0f] */)
{
  // FIXME: Slow
  contrast = std::clamp(((contrast + 1.0f) / 2.0f), 0.0f, 1.0f);
  float const factor = static_cast<float>(tan(contrast * std::numbers::pi_v<float> / 2.0f));
  for(int y = 0; y < src.get_height(); ++y) {
    for(int x = 0; x < src.get_width(); ++x) {
      Color rgba = src.get_pixel_color({x, y});
      rgba.r = (rgba.r - 0.5f) * factor + 0.5f;
      rgba.g = (rgba.g - 0.5f) * factor + 0.5f;
      rgba.b = (rgba.b - 0.5f) * factor + 0.5f;
      src.put_pixel_color({x, y}, clamp(rgba));
    }
  }
}

template<typename Pixel>
void apply_invert(PixelView<Pixel>& src)
{
  // FIXME: Slow
  for(int y = 0; y < src.get_height(); ++y) {
    for(int x = 0; x < src.get_width(); ++x) {
      Color rgba = src.get_pixel_color({x, y});
      rgba.r = 1.0f - rgba.r;
      rgba.g = 1.0f - rgba.g;
      rgba.b = 1.0f - rgba.b;
      src.put_pixel_color({x, y}, clamp(rgba));
    }
  }
}

/** Apply \a lut to the color channels, \a lut must have
    Pixel::max() + 1 entries and match the pixel's value_type */
template<typename Pixel, typename T>
void apply_lut(PixelView<Pixel>& src, T const* lut)
{
  if constexpr (!std::is_same<T, typename Pixel::value_type>::value ||
                !std::is_integral<T>::value) {
    throw std::invalid_argument("apply_lut(): lut type doesn't match the PixelFormat");
  } else {
    for_each_pixel(src, [lut](Pixel& pixel) {
      pixel = make_pixel<Pixel>(lut[red(pixel)],
                                lut[green(pixel)],
                                lut[blue(pixel)],
                                alpha(pixel));
    });
  }
}

template<typename Pixel>
void apply_threshold(PixelView<Pixel>& src, Color threshold)
{
  using type = typename Pixel::value_type;

  type const min_val = 0;
  type const max_val = Pixel::max();

  type const irthreshold = f2value<Pixel>(threshold.r);
  type const igthreshold = f2value<Pixel>(threshold.g);
  type const ibthreshold = f2value<Pixel>(threshold.b);

  for_each_pixel(src, [&](Pixel& pixel) {
    pixel = make_pixel<Pixel>(red(pixel) > irthreshold ? max_val : min_val,
                              green(pixel) > igthreshold ? max_val : min_val,
                              blue(pixel) > ibthreshold ? max_val : min_val,
                              alpha(pixel));
  });
}

template<typename Pixel>
void apply_grayscale(PixelView<Pixel>& src)
{
  using type = typename Pixel::value_type;
  using acc = accumulate_t<type>;

  for_each_pixel(src, [](Pixel& pixel) {
    type const v = static_cast<type>((acc(red(pixel)) + green(pixel) + blue(pixel)) / 3);
    pixel = make_pixel<Pixel>(v, v, v, alpha(pixel));
  });
}

template<typename Pixel>
void apply_hsv(PixelView<Pixel>& src, float hue, float saturation, float value)
{
  // FIXME: Slow
  for(int y = 0; y < src.get_height(); ++y) {
    for(int x = 0; x < src.get_width(); ++x) {
      Color color = src.get_pixel_color({x, y});
      HSVColor hsv = hsv_from_color(color);

      hsv.hue += hue;
      hsv.saturation += saturation;
      hsv.value += value;

      hsv.hue = hsv.hue - std::floor(hsv.hue);
      hsv.saturation = std::clamp(hsv.saturation, 0.0f, 1.0f);
      hsv.value = std::clamp(hsv.value, 0.0f, 1.0f);

      Color result = color_from_hsv(hsv);
      result.a = color.a;
      src.put_pixel_color({x, y}, result);
    }
  }
}

namespace detail {

inline int positive_mod(int i, int n) {
  return (i % n + n) % n;
}

} // namespace detail

template<typename Pixel>
void apply_offset(PixelView<Pixel>& src, geom::ioffset const& offset)
{
  if (src.get_size().is_empty()) {
    return;
  }

  PixelData<Pixel> copy(src);

  geom::ipoint const pos(detail::positive_mod(offset.x(), src.get_size().width()),
                         detail::positive_mod(offset.y(), src.get_size().height()));
  geom::isize const size(src.get_size());

  blit(copy, src, pos + geom::ioffset(0, -size.height()));
  blit(copy, src, pos + geom::ioffset(-size.width(), -size.height()));
  blit(copy, src, pos + geom::ioffset(0, 0));
  blit(copy, src, pos + geom::ioffset(-size.width(), 0));
}

SOFTWARE_SURFACE_LIFT_VOID(apply_gamma)
SOFTWARE_SURFACE_LIFT_VOID(apply_multiply)
SOFTWARE_SURFACE_LIFT_VOID(apply_add)
SOFTWARE_SURFACE_LIFT_VOID(apply_brightness)
SOFTWARE_SURFACE_LIFT_VOID(apply_contrast)
SOFTWARE_SURFACE_LIFT_VOID(apply_invert)
SOFTWARE_SURFACE_LIFT_VOID(apply_lut)
SOFTWARE_SURFACE_LIFT_VOID(apply_threshold)
SOFTWARE_SURFACE_LIFT_VOID(apply_grayscale)
SOFTWARE_SURFACE_LIFT_VOID(apply_hsv)
SOFTWARE_SURFACE_LIFT_VOID(apply_offset)

} // namespace surf

#endif

/* EOF */
