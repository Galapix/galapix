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

#ifndef HEADER_SURF_FWD_HPP
#define HEADER_SURF_FWD_HPP

#include <stdint.h>

namespace surf {

enum class Transform;

template<typename T> struct tRGBPixel;
template<typename T> struct tRGBAPixel;
template<typename T> struct tLPixel;
template<typename T> struct tLAPixel;

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

using RGBPixel = RGB8Pixel;
using RGBAPixel = RGBA8Pixel;

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

class Color;
class IPixelData;
class SoftwareSurface;
class SoftwareSurfaceFactory;
class SoftwareSurfaceLoader;

enum class BlendFunc;
enum class PixelFormat;

template<typename Pixel> class PixelData;
template<typename Pixel> class PixelView;

} // namespace surf

#endif

/* EOF */
