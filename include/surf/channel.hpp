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

#ifndef HEADER_SURF_CHANNEL_HPP
#define HEADER_SURF_CHANNEL_HPP

#include <stdexcept>
#include <vector>

#include "fwd.hpp"
#include "pixel.hpp"
#include "pixel_data.hpp"
#include "pixel_view.hpp"

namespace surf {

template<typename T>
PixelData<tRGBPixel<T>> join_channel(PixelView<tLPixel<T>> const& red,
                                     PixelView<tLPixel<T>> const& green,
                                     PixelView<tLPixel<T>> const& blue)
{
  geom::isize const size = red.get_size();
  if (size != green.get_size() ||
      size != blue.get_size()) {
    throw std::invalid_argument("channels must be the same size");
  }

  PixelData<tRGBPixel<T>> dst(size);

  for (int y = 0; y < size.height(); ++y) {
    tRGBPixel<T>* dst_r = dst.get_row(y);
    tLPixel<T> const* red_r = red.get_row(y);
    tLPixel<T> const* green_r = green.get_row(y);
    tLPixel<T> const* blue_r = blue.get_row(y);

    for (int x = 0; x < size.width(); ++x) {
      dst_r[x] = make_pixel<tRGBPixel<T>>(red_r[x].l, green_r[x].l, blue_r[x].l);
    }
  }

  return dst;
}

template<typename T>
PixelData<tRGBAPixel<T>> join_channel(PixelView<tLPixel<T>> const& red,
                                      PixelView<tLPixel<T>> const& green,
                                      PixelView<tLPixel<T>> const& blue,
                                      PixelView<tLPixel<T>> const& alpha)
{
  geom::isize const size = red.get_size();
  if (size != green.get_size() ||
      size != blue.get_size() ||
      size != alpha.get_size()) {
    throw std::invalid_argument("channels must be the same size");
  }

  PixelData<tRGBAPixel<T>> dst(size);

  for (int y = 0; y < size.height(); ++y) {
    tRGBAPixel<T>* dst_r = dst.get_row(y);
    tLPixel<T> const* red_r = red.get_row(y);
    tLPixel<T> const* green_r = green.get_row(y);
    tLPixel<T> const* blue_r = blue.get_row(y);
    tLPixel<T> const* alpha_r = alpha.get_row(y);

    for (int x = 0; x < size.width(); ++x) {
      dst_r[x] = make_pixel<tRGBAPixel<T>>(red_r[x].l, green_r[x].l, blue_r[x].l, alpha_r[x].l);
    }
  }

  return dst;
}

template<typename T>
std::vector<PixelData<tLPixel<T>>>
split_channel(PixelView<tRGBPixel<T>> const& src)
{
  geom::isize const size = src.get_size();

  PixelData<tLPixel<T>> red_c(size);
  PixelData<tLPixel<T>> green_c(size);
  PixelData<tLPixel<T>> blue_c(size);

  for (int y = 0; y < size.height(); ++y) {
    tRGBPixel<T> const* src_r = src.get_row(y);
    tLPixel<T>* red_r = red_c.get_row(y);
    tLPixel<T>* green_r = green_c.get_row(y);
    tLPixel<T>* blue_r = blue_c.get_row(y);

    for (int x = 0; x < size.width(); ++x) {
      red_r[x] = tLPixel<T>{red(src_r[x])};
      green_r[x] = tLPixel<T>{green(src_r[x])};
      blue_r[x] = tLPixel<T>{blue(src_r[x])};
    }
  }

  return {red_c, green_c, blue_c};
}

template<typename T>
std::vector<PixelData<tLPixel<T>>>
split_channel(PixelView<tRGBAPixel<T>> const& src)
{
  geom::isize const size = src.get_size();

  PixelData<tLPixel<T>> red_c(size);
  PixelData<tLPixel<T>> green_c(size);
  PixelData<tLPixel<T>> blue_c(size);
  PixelData<tLPixel<T>> alpha_c(size);

  for (int y = 0; y < size.height(); ++y) {
    tRGBAPixel<T> const* src_r = src.get_row(y);
    tLPixel<T>* red_r = red_c.get_row(y);
    tLPixel<T>* green_r = green_c.get_row(y);
    tLPixel<T>* blue_r = blue_c.get_row(y);
    tLPixel<T>* alpha_r = alpha_c.get_row(y);

    for (int x = 0; x < size.width(); ++x) {
      red_r[x] = tLPixel<T>{red(src_r[x])};
      green_r[x] = tLPixel<T>{green(src_r[x])};
      blue_r[x] = tLPixel<T>{blue(src_r[x])};
      alpha_r[x] = tLPixel<T>{alpha(src_r[x])};
    }
  }

  return {red_c, green_c, blue_c, alpha_c};
}

std::vector<SoftwareSurface> split_channel(SoftwareSurface const& src);
SoftwareSurface join_channel(std::vector<SoftwareSurface> channels);

} // namespace surf

#endif

/* EOF */
