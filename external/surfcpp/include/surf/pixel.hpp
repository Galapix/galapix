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

#ifndef HEADER_SURF_PIXEL_HPP
#define HEADER_SURF_PIXEL_HPP

#include <stdint.h>

#include <algorithm>
#include <bit>
#include <limits>

#include "pixel_format.hpp"
#include "promote.hpp"

namespace surf {

template<typename T>
struct tRGBPixel
{
  using value_type = T;
  static constexpr bool has_alpha() { return false; }
  static constexpr bool has_rgb() { return true; }
  static constexpr T max() {
    if constexpr (std::is_floating_point<T>::value) {
      return 1.0;
    } else {
      return std::numeric_limits<T>::max();
    }
  }
  static constexpr bool is_floating_point() { return std::is_floating_point<T>::value; }

  T r;
  T g;
  T b;

  inline bool operator==(tRGBPixel<T> const& rhs) const = default;
};

template<typename T>
struct tRGBAPixel
{
  using value_type = T;
  static constexpr bool has_alpha() { return true; }
  static constexpr bool has_rgb() { return true; }
  static constexpr T max() {
    if constexpr (std::is_floating_point<T>::value) {
      return 1.0;
    } else {
      return std::numeric_limits<T>::max();
    }
  }
  static constexpr bool is_floating_point() { return std::is_floating_point<T>::value; }

  T r;
  T g;
  T b;
  T a;

  inline bool operator==(tRGBAPixel<T> const& rhs) const = default;
};

template<typename T>
struct tLPixel
{
  using value_type = T;
  static constexpr bool has_alpha() { return false; }
  static constexpr bool has_rgb() { return false; }
  static constexpr T max() {
    if constexpr (std::is_floating_point<T>::value) {
      return 1.0;
    } else {
      return std::numeric_limits<T>::max();
    }
  }
  static constexpr bool is_floating_point() { return std::is_floating_point<T>::value; }

  T l;

  inline bool operator==(tLPixel<T> const& rhs) const = default;
};

template<typename T>
struct tLAPixel
{
  using value_type = T;
  static constexpr bool has_alpha() { return true; }
  static constexpr bool has_rgb() { return false; }
  static constexpr T max() {
    if constexpr (std::is_floating_point<T>::value) {
      return 1.0;
    } else {
      return std::numeric_limits<T>::max();
    }
  }
  static constexpr bool is_floating_point() { return std::is_floating_point<T>::value; }

  T l;
  T a;

  inline bool operator==(tLAPixel<T> const& rhs) const = default;
};

using RGB8Pixel = tRGBPixel<uint8_t>;
using RGBA8Pixel = tRGBAPixel<uint8_t>;

using RGB16Pixel = tRGBPixel<uint16_t>;
using RGBA16Pixel = tRGBAPixel<uint16_t>;

using RGB32Pixel = tRGBPixel<uint32_t>;
using RGBA32Pixel = tRGBAPixel<uint32_t>;

using RGB32fPixel = tRGBPixel<float>;
using RGBA32fPixel = tRGBAPixel<float>;

using RGB64fPixel = tRGBPixel<double>;
using RGBA64fPixel = tRGBAPixel<double>;

using L8Pixel = tLPixel<uint8_t>;
using LA8Pixel = tLAPixel<uint8_t>;

using L16Pixel = tLPixel<uint16_t>;
using LA16Pixel = tLAPixel<uint16_t>;

using L32Pixel = tLPixel<uint32_t>;
using LA32Pixel = tLAPixel<uint32_t>;

using L32fPixel = tLPixel<float>;
using LA32fPixel = tLAPixel<float>;

using L64fPixel = tLPixel<double>;
using LA64fPixel = tLAPixel<double>;

template<typename Pixel> inline
constexpr Pixel make_pixel(typename Pixel::value_type r,
                           typename Pixel::value_type g,
                           typename Pixel::value_type b,
                           typename Pixel::value_type a = Pixel::max())
{
  if constexpr (Pixel::has_rgb()) {
    if constexpr (Pixel::has_alpha()) {
      return Pixel{r, g, b, a};
    } else {
      return Pixel{r, g, b};
    }
  } else {
    using acc = accumulate_t<typename Pixel::value_type>;
    auto const l = static_cast<typename Pixel::value_type>((acc(r) + g + b) / 3);
    if constexpr (Pixel::has_alpha()) {
      return Pixel{l, a};
    } else {
      return Pixel{l};
    }
  }
}

template<typename Pixel> constexpr inline typename Pixel::value_type red(Pixel pixel)
{
  if constexpr (Pixel::has_rgb()) {
    return pixel.r;
  } else {
    return pixel.l;
  }
}

template<typename Pixel> constexpr inline typename Pixel::value_type green(Pixel pixel)
{
  if constexpr (Pixel::has_rgb()) {
    return pixel.g;
  } else {
    return pixel.l;
  }
}

template<typename Pixel> constexpr inline typename Pixel::value_type blue(Pixel pixel)
{
  if constexpr (Pixel::has_rgb()) {
    return pixel.b;
  } else {
    return pixel.l;
  }
}

template<typename Pixel> constexpr inline typename Pixel::value_type alpha(Pixel pixel) {
  if constexpr (Pixel::has_alpha()) {
    return pixel.a;
  } else {
    return Pixel::max();
  }
}

template<typename Pixel> constexpr inline float red_f(Pixel pixel) { return static_cast<float>(red(pixel)) / static_cast<float>(Pixel::max()); }
template<typename Pixel> constexpr inline float green_f(Pixel pixel) { return static_cast<float>(green(pixel)) / static_cast<float>(Pixel::max()); }
template<typename Pixel> constexpr inline float blue_f(Pixel pixel) { return static_cast<float>(blue(pixel)) / static_cast<float>(Pixel::max()); }

template<typename Pixel> constexpr inline float alpha_f(Pixel pixel) {
  if constexpr (Pixel::has_alpha()) {
    return static_cast<float>(pixel.a) / static_cast<float>(Pixel::max());
  } else {
    return 1.0f;
  }
}

template<typename DstPixel> inline
typename DstPixel::value_type f2value(float v)
{
  using srctype = float;
  using dsttype = typename DstPixel::value_type;

  if constexpr (std::is_integral<dsttype>::value) {
    v = std::clamp(v, 0.0f, 1.0f);
    if constexpr (sizeof(dsttype) == sizeof(srctype)) {
      // special case, as uint32 -> float32 will overflow
      return static_cast<dsttype>(
        std::min(static_cast<uint64_t>(v * static_cast<srctype>(DstPixel::max()) + 0.5f),
                 static_cast<uint64_t>(DstPixel::max())));
    } else {
      return static_cast<dsttype>(v * static_cast<srctype>(DstPixel::max()) + 0.5f);
    }
  } else {
    return static_cast<dsttype>(v * static_cast<srctype>(DstPixel::max()));
  }
}

template<typename Pixel, typename T> inline
constexpr typename Pixel::value_type clamp_pixel(T v)
{
  return static_cast<typename Pixel::value_type>(std::clamp<T>(v, 0, static_cast<T>(Pixel::max())));
}

template<typename Pixel, typename T> inline
constexpr typename Pixel::value_type clamp_pixel_max(T v)
{
  return static_cast<typename Pixel::value_type>(std::min<T>(v, static_cast<T>(Pixel::max())));
}

template<typename Pixel>
struct PPixelFormat
{
};

template<>
struct PPixelFormat<RGB8Pixel>
{
  static constexpr PixelFormat format = PixelFormat::RGB8;
  static constexpr int bits_per_pixel = 24;
  static constexpr int bytes_per_pixel = 3;
  static constexpr uint32_t rmask = std::endian::native == std::endian::big ? 0x00ff0000 : 0x000000ff;
  static constexpr uint32_t gmask = std::endian::native == std::endian::big ? 0x0000ff00 : 0x0000ff00; // NOLINT
  static constexpr uint32_t bmask = std::endian::native == std::endian::big ? 0x000000ff : 0x00ff0000;
  static constexpr uint32_t amask = 0x00000000;
};

template<>
struct PPixelFormat<RGBA8Pixel>
{
  static constexpr PixelFormat format = PixelFormat::RGBA8;
  static constexpr int bits_per_pixel = 32;
  static constexpr int bytes_per_pixel = 4;
  static constexpr uint32_t rmask = std::endian::native == std::endian::big ? 0xff000000 : 0x000000ff;
  static constexpr uint32_t gmask = std::endian::native == std::endian::big ? 0x00ff0000 : 0x0000ff00;
  static constexpr uint32_t bmask = std::endian::native == std::endian::big ? 0x0000ff00 : 0x00ff0000;
  static constexpr uint32_t amask = std::endian::native == std::endian::big ? 0x000000ff : 0xff000000;
};

template<>
struct PPixelFormat<RGB16Pixel>
{
  static constexpr PixelFormat format = PixelFormat::RGB16;
  static constexpr int bits_per_pixel = 48;
  static constexpr int bytes_per_pixel = 6;
};

template<>
struct PPixelFormat<RGBA16Pixel>
{
  static constexpr PixelFormat format = PixelFormat::RGBA16;
  static constexpr int bits_per_pixel = 64;
  static constexpr int bytes_per_pixel = 8;
};

template<>
struct PPixelFormat<RGB32Pixel>
{
  static constexpr PixelFormat format = PixelFormat::RGB32;
  static constexpr int bits_per_pixel = 96;
  static constexpr int bytes_per_pixel = 12;
};

template<>
struct PPixelFormat<RGBA32Pixel>
{
  static constexpr PixelFormat format = PixelFormat::RGBA32;
  static constexpr int bits_per_pixel = 128;
  static constexpr int bytes_per_pixel = 16;
};

template<>
struct PPixelFormat<RGB32fPixel>
{
  static constexpr PixelFormat format = PixelFormat::RGB32f;
  static constexpr int bits_per_pixel = 96;
  static constexpr int bytes_per_pixel = 12;
};

template<>
struct PPixelFormat<RGBA32fPixel>
{
  static constexpr PixelFormat format = PixelFormat::RGBA32f;
  static constexpr int bits_per_pixel = 128;
  static constexpr int bytes_per_pixel = 16;
};

template<>
struct PPixelFormat<RGB64fPixel>
{
  static constexpr PixelFormat format = PixelFormat::RGB64f;
  static constexpr int bits_per_pixel = 192;
  static constexpr int bytes_per_pixel = 24;
};

template<>
struct PPixelFormat<RGBA64fPixel>
{
  static constexpr PixelFormat format = PixelFormat::RGBA64f;
  static constexpr int bits_per_pixel = 256;
  static constexpr int bytes_per_pixel = 32;
};

template<>
struct PPixelFormat<L8Pixel>
{
  static constexpr PixelFormat format = PixelFormat::L8;
  static constexpr int bits_per_pixel = 8;
  static constexpr int bytes_per_pixel = 1;
  static constexpr uint32_t rmask = std::endian::native == std::endian::big ? 0xff000000 : 0x000000ff;
  static constexpr uint32_t gmask = std::endian::native == std::endian::big ? 0xff000000 : 0x000000ff;
  static constexpr uint32_t bmask = std::endian::native == std::endian::big ? 0xff000000 : 0x000000ff;
  static constexpr uint32_t amask = std::endian::native == std::endian::big ? 0xff000000 : 0x000000ff;
};


template<>
struct PPixelFormat<LA8Pixel>
{
  static constexpr PixelFormat format = PixelFormat::LA8;
  static constexpr int bits_per_pixel = 16;
  static constexpr int bytes_per_pixel = 2;
  static constexpr uint32_t rmask = std::endian::native == std::endian::big ? 0xff000000 : 0x000000ff;
  static constexpr uint32_t gmask = std::endian::native == std::endian::big ? 0xff000000 : 0x000000ff;
  static constexpr uint32_t bmask = std::endian::native == std::endian::big ? 0xff000000 : 0x000000ff;
  static constexpr uint32_t amask = std::endian::native == std::endian::big ? 0x00ff0000 : 0x0000ff00;
};

template<>
struct PPixelFormat<L16Pixel>
{
  static constexpr PixelFormat format = PixelFormat::L16;
};

template<>
struct PPixelFormat<LA16Pixel>
{
  static constexpr PixelFormat format = PixelFormat::LA16;
};

template<>
struct PPixelFormat<L32Pixel>
{
  static constexpr PixelFormat format = PixelFormat::L32;
};

template<>
struct PPixelFormat<LA32Pixel>
{
  static constexpr PixelFormat format = PixelFormat::LA32;
};

template<>
struct PPixelFormat<L32fPixel>
{
  static constexpr PixelFormat format = PixelFormat::L32f;
};

template<>
struct PPixelFormat<LA32fPixel>
{
  static constexpr PixelFormat format = PixelFormat::LA32f;
};

template<>
struct PPixelFormat<L64fPixel>
{
  static constexpr PixelFormat format = PixelFormat::L64f;
};

template<>
struct PPixelFormat<LA64fPixel>
{
  static constexpr PixelFormat format = PixelFormat::LA64f;
};

} // namespace surf

#endif

/* EOF */
