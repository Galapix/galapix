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

#ifndef HEADER_SURF_TRANSFORM_HPP
#define HEADER_SURF_TRANSFORM_HPP

#include <geom/size.hpp>

#include "pixel_data.hpp"
#include "software_surface.hpp"
#include "unwrap.hpp"

namespace surf {

enum class Transform
{
  ROTATE_0,
  ROTATE_90,
  ROTATE_180,
  ROTATE_270,
  ROTATE_0_FLIP,
  ROTATE_90_FLIP,
  ROTATE_180_FLIP,
  ROTATE_270_FLIP,

  FLIP_VERTICAL = ROTATE_0_FLIP,
  FLIP_HORIZONTAL = ROTATE_180_FLIP,
};

Transform transform_from_string(std::string_view text);

template<typename Pixel>
PixelData<Pixel> transform(PixelView<Pixel> const& src, Transform mod)
{
  switch (mod)
  {
    case Transform::ROTATE_0:
      return src;

    case Transform::ROTATE_90:
      return rotate90(src);

    case Transform::ROTATE_180:
      return rotate180(src);

    case Transform::ROTATE_270:
      return rotate270(src);

    case Transform::ROTATE_0_FLIP:
      return flip_vertical(src);

    case Transform::ROTATE_90_FLIP:
      // FIXME: Could be made faster
      return flip_vertical(rotate90(src));

    case Transform::ROTATE_180_FLIP:
      return flip_horizontal(src);

    case Transform::ROTATE_270_FLIP:
      // FIXME: Could be made faster
      return flip_vertical(rotate270(src));

    default:
      assert(false && "never reached");
      return src;
  }
}

template<typename Pixel>
PixelData<Pixel> rotate90(PixelView<Pixel> const& src)
{
  PixelData<Pixel> dst(geom::isize(src.get_size().height(), src.get_size().width()));

  for(int y = 0; y < src.get_size().height(); ++y) {
    for(int x = 0; x < src.get_size().width(); ++x) {
      dst.put_pixel(geom::ipoint(src.get_size().height() - y - 1, x),
                    src.get_pixel(geom::ipoint(x, y)));
    }
  }

  return dst;
}

template<typename Pixel>
PixelData<Pixel> rotate180(PixelView<Pixel> const& src)
{
  PixelData<Pixel> dst(src.get_size());

  for(int y = 0; y < src.get_size().height(); ++y) {
    for(int x = 0; x < src.get_size().width(); ++x) {
      dst.put_pixel(geom::ipoint(src.get_size().width() - x - 1, src.get_size().height() - 1 - y),
                    src.get_pixel(geom::ipoint(x, y)));
    }
  }

  return dst;
}

template<typename Pixel>
PixelData<Pixel> rotate270(PixelView<Pixel> const& src)
{
  PixelData<Pixel> dst(geom::isize(src.get_size().height(), src.get_size().width()));

  for(int y = 0; y < src.get_size().height(); ++y) {
    for(int x = 0; x < src.get_size().width(); ++x) {
      dst.put_pixel(geom::ipoint(y, src.get_size().width() - 1 - x),
                    src.get_pixel(geom::ipoint(x, y)));
    }
  }

  return dst;
}

template<typename Pixel>
PixelData<Pixel> flip_vertical(PixelView<Pixel> const& src)
{
  PixelData<Pixel> dst(src.get_size());

  for(int y = 0; y < src.get_size().height(); ++y) {
    std::copy_n(src.get_row(y),
                src.get_width(),
                dst.get_row(src.get_size().height() - 1 - y));
  }

  return dst;
}

template<typename Pixel>
PixelData<Pixel> flip_horizontal(PixelView<Pixel> const& src)
{
  PixelData<Pixel> dst(src.get_size());

  for(int y = 0; y < src.get_size().height(); ++y) {
    for(int x = 0; x < src.get_size().width(); ++x) {
      dst.put_pixel(geom::ipoint(src.get_size().width() - 1 - x, y),
                    src.get_pixel(geom::ipoint(x, y)));
    }
  }

  return dst;
}

template<typename Pixel>
PixelData<Pixel> halve(PixelView<Pixel> const& src)
{
  using type = typename Pixel::value_type;
  using acc = accumulate_t<type>;

  PixelData<Pixel> dst(src.get_size() / 2);

  for(int y = 0; y < dst.get_height(); ++y) {
    for(int x = 0; x < dst.get_width(); ++x) {
      Pixel const src1 = src.get_pixel(geom::ipoint(x * 2 + 0, y * 2 + 0));
      Pixel const src2 = src.get_pixel(geom::ipoint(x * 2 + 1, y * 2 + 0));
      Pixel const src3 = src.get_pixel(geom::ipoint(x * 2 + 0, y * 2 + 1));
      Pixel const src4 = src.get_pixel(geom::ipoint(x * 2 + 1, y * 2 + 1));

      Pixel const dstpixel = make_pixel<Pixel>(static_cast<type>((acc(red(src1)) + red(src2) + red(src3) + red(src4)) / 4),
                                               static_cast<type>((acc(green(src1)) + green(src2) + green(src3) + green(src4)) / 4),
                                               static_cast<type>((acc(blue(src1)) + blue(src2) + blue(src3) + blue(src4)) / 4),
                                               static_cast<type>((acc(alpha(src1)) + alpha(src2) + alpha(src3) + alpha(src4)) / 4));
      dst.put_pixel(geom::ipoint(x, y), dstpixel);
    }
  }

  return dst;
}

template<typename Pixel>
PixelData<Pixel> scale(PixelView<Pixel> const& src, geom::isize const& size)
{
  if (src.get_size() == size) { return src; }
  if (src.get_size() == geom::isize(0, 0)) { return PixelData<Pixel>(size); }

  PixelData<Pixel> dst(size);

  for(int y = 0; y < dst.get_height(); ++y) {
    for(int x = 0; x < dst.get_width(); ++x) {
      dst.put_pixel({x, y},
                    src.get_pixel(geom::ipoint(x * src.get_size().width()  / dst.get_size().width(),
                                               y * src.get_size().height() / dst.get_size().height())));
    }
  }
  return dst;
}

template<typename Pixel>
PixelData<Pixel> crop(PixelView<Pixel> const& src, geom::irect const& rect)
{
  // Clip the rectangle to the image
  geom::irect clipped(std::clamp(rect.left(), 0, src.get_width()),
                      std::clamp(rect.top(), 0, src.get_height()),
                      std::clamp(rect.right(), 0, src.get_width()),
                      std::clamp(rect.bottom(), 0, src.get_height()));

  PixelData<Pixel> dst(clipped.size());

  for(int y = clipped.top(); y < clipped.bottom(); ++y) {
    std::copy_n(src.get_row(y) + clipped.left(),
                clipped.width(),
                dst.get_row(y - clipped.top()));
  }

  return dst;
}

SOFTWARE_SURFACE_LIFT(transform)
SOFTWARE_SURFACE_LIFT(rotate90)
SOFTWARE_SURFACE_LIFT(rotate180)
SOFTWARE_SURFACE_LIFT(rotate270)
SOFTWARE_SURFACE_LIFT(flip_horizontal)
SOFTWARE_SURFACE_LIFT(flip_vertical)

SOFTWARE_SURFACE_LIFT(halve)
SOFTWARE_SURFACE_LIFT(scale)
SOFTWARE_SURFACE_LIFT(crop)

} // namespace surf

#endif

/* EOF */
