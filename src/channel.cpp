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

#include "channel.hpp"

#include "software_surface.hpp"
#include "unwrap.hpp"

namespace surf {

namespace {

template<typename T>
std::vector<SoftwareSurface>
split_channel_to_software_surface(PixelView<tLPixel<T>> const& src)
{
  return {SoftwareSurface(src)};
}

template<typename T>
std::vector<SoftwareSurface>
split_channel_to_software_surface(PixelView<tLAPixel<T>> const& src)
{
  return {SoftwareSurface(src)};
}

template<typename T>
std::vector<SoftwareSurface>
split_channel_to_software_surface(PixelView<tRGBPixel<T>> const& src)
{
  auto res = split_channel(src);
  return {
    SoftwareSurface(std::move(res[0])),
    SoftwareSurface(std::move(res[1])),
    SoftwareSurface(std::move(res[2]))
  };
}

template<typename T>
std::vector<SoftwareSurface>
split_channel_to_software_surface(PixelView<tRGBAPixel<T>> const& src)
{
  auto res = split_channel(src);
  return {
    SoftwareSurface(std::move(res[0])),
    SoftwareSurface(std::move(res[1])),
    SoftwareSurface(std::move(res[2])),
    SoftwareSurface(std::move(res[3]))
  };
}

template<typename T>
SoftwareSurface join_channel_to_software_surface(PixelView<tLPixel<T>> red_c,
                                                 PixelView<tLPixel<T>> green_c,
                                                 PixelView<tLPixel<T>> blue_c)
{
  return SoftwareSurface(join_channel(red_c, green_c, blue_c));
}

template<typename T>
SoftwareSurface join_channel_to_software_surface(PixelView<tLPixel<T>> red_c,
                                                 PixelView<tLPixel<T>> green_c,
                                                 PixelView<tLPixel<T>> blue_c,
                                                 PixelView<tLPixel<T>> alpha_c)
{
  return SoftwareSurface(join_channel(red_c, green_c, blue_c, alpha_c));
}

} // namespace

std::vector<SoftwareSurface>
split_channel(SoftwareSurface const& src)
{
  PIXELFORMAT_TO_TYPE(
    src.get_format(), srctype,
    return split_channel_to_software_surface(src.as_pixelview<srctype>()););
}

SoftwareSurface
join_channel(std::vector<SoftwareSurface> channels)
{
  if (channels.size() == 1) {
    return channels[0];
  } else if (channels.size() == 3) {
      PIXELFORMAT_TO_TYPE(
        channels[0].get_format(), srctype,
        return join_channel_to_software_surface(channels[0].as_pixelview<tLPixel<typename srctype::value_type>>(),
                                                channels[1].as_pixelview<tLPixel<typename srctype::value_type>>(),
                                                channels[2].as_pixelview<tLPixel<typename srctype::value_type>>()));
  } else if (channels.size() == 4) {
      PIXELFORMAT_TO_TYPE(
        channels[0].get_format(), srctype,
        return join_channel_to_software_surface(channels[0].as_pixelview<tLPixel<typename srctype::value_type>>(),
                                                channels[1].as_pixelview<tLPixel<typename srctype::value_type>>(),
                                                channels[2].as_pixelview<tLPixel<typename srctype::value_type>>(),
                                                channels[3].as_pixelview<tLPixel<typename srctype::value_type>>()));
  } else {
    throw std::invalid_argument("invalid number of channels");
  }
}

} // namespace surf

/* EOF */
