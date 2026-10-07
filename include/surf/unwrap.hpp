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

#ifndef HEADER_SURF_UNWRAP_HPP
#define HEADER_SURF_UNWRAP_HPP

#include <stdexcept>

#include "software_surface.hpp"

#define PIXELFORMAT_TO_TYPE__CASE(type, expr, value, type_value)        \
  case value: {                                                         \
    using type = type_value; /* NOLINT */                               \
    expr;                                                               \
    break;                                                              \
  }

#define PIXELFORMAT_TO_TYPE(format, type, expr)                 \
  do {                                                          \
    switch (format)                                             \
    {                                                           \
      default:                                                  \
      case PixelFormat::NONE: {                                 \
        throw std::invalid_argument("unknown PixelFormat");     \
        break;                                                  \
      }                                                         \
                                                                \
      /* see the #if 0 below */                                 \
      case PixelFormat::RGB64f:                                 \
      case PixelFormat::RGBA64f:                                \
      case PixelFormat::L64f:                                   \
      case PixelFormat::LA64f: {                                \
        throw std::invalid_argument(                            \
          "64-bit float PixelFormats are not supported by SoftwareSurface"); \
        break;                                                  \
      }                                                         \
                                                                \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::RGB8, RGB8Pixel)              \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::RGBA8, RGBA8Pixel)            \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::RGB16, RGB16Pixel)            \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::RGBA16, RGBA16Pixel)          \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::RGB32, RGB32Pixel)            \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::RGBA32, RGBA32Pixel)          \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::RGB32f, RGB32fPixel)          \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::RGBA32f, RGBA32fPixel)        \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::L8, L8Pixel)                  \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::LA8, LA8Pixel)                \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::L16, L16Pixel)                \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::LA16, LA16Pixel)              \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::L32, L32Pixel)                \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::LA32, LA32Pixel)              \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::L32f, L32fPixel)              \
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::LA32f, LA32fPixel)            \
    }                                                                                \
  } while (false)

#if 0
// Disable 64-bit floating point support, overkill in most situation.
// Reduces library size from 4.6MB to 3.2MB
// Reduces compile time from to 1:55m to 1:22m
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::RGB64f, RGB64fPixel)
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::RGBA64f, RGBA64fPixel)
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::L64f, L64fPixel)
    PIXELFORMAT_TO_TYPE__CASE(type, expr, PixelFormat::LA64f, LA64fPixel)
#endif

#define PIXELFORMAT2_TO_TYPE(srcformat, srctype, dstformat, dsttype, expr) \
  PIXELFORMAT_TO_TYPE(                                                  \
    (srcformat),                                                        \
    srctype,                                                            \
    PIXELFORMAT_TO_TYPE(                                                \
      (dstformat),                                                      \
      dsttype,                                                          \
      expr))

#define SOFTWARE_SURFACE_LIFT_VOID(function)                        \
  template<typename ...Args>                                        \
  void function(SoftwareSurface& src,                               \
                Args&&... args)                                     \
  {                                                                 \
    PIXELFORMAT_TO_TYPE(                                            \
      src.get_format(), srctype,                                    \
      function(src.as_pixelview<srctype>(),                         \
               std::forward<Args>(args)...));                       \
  }

#define SOFTWARE_SURFACE_LIFT(function)                        \
  template<typename ...Args>                                   \
  SoftwareSurface function(SoftwareSurface const& src,         \
                           Args&&... args)                     \
  {                                                            \
    PIXELFORMAT_TO_TYPE(                                       \
      src.get_format(), srctype,                               \
      return SoftwareSurface(                                  \
        function(src.as_pixelview<srctype>(),                  \
                 std::forward<Args>(args)...)));               \
  }

#define SOFTWARE_SURFACE_LIFT2(function)                                \
  template<typename ...Args>                                            \
  SoftwareSurface function(SoftwareSurface const& src,                  \
                           SoftwareSurface const& dst,                  \
                           Args&&... args)                              \
  {                                                                     \
    PIXELFORMAT2_TO_TYPE(                                               \
      src.get_format(), srctype,                                        \
      dst.get_format(), dsttype,                                        \
      function(src.as_pixelview<srctype>(),                             \
               dst.as_pixelview<dsttype>(),                             \
               std::forward<Args>(args)...))                            \
      );                                                                \
  }

#endif

/* EOF */
