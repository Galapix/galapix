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

#ifndef HEADER_SURF_BLENDFUNC_HPP
#define HEADER_SURF_BLENDFUNC_HPP

#include <stdexcept>
#include <string_view>

#include "blend.hpp"

namespace surf {

enum class BlendFunc
{
  COPY,
  BLEND,
  ADD,
  MULTIPLY,
};

BlendFunc BlendFunc_from_string(std::string_view blendfunc_str);

#define BLENDFUNC_TO_TYPE(blendfunc, blendfunc_t, expr)                 \
  do {                                                                  \
    switch (blendfunc) {                                                \
      case BlendFunc::COPY: {                                           \
        using blendfunc_t = pixel_copy<srctype, dsttype>; /* NOLINT */  \
        expr;                                                           \
        break;                                                          \
      }                                                                 \
      case BlendFunc::ADD: {                                            \
        using blendfunc_t = pixel_add<srctype, dsttype>; /* NOLINT */   \
        expr;                                                           \
        break;                                                          \
      }                                                                 \
      case BlendFunc::BLEND: {                                          \
        using blendfunc_t = pixel_blend<srctype, dsttype>; /* NOLINT */ \
        expr;                                                           \
        break;                                                          \
      }                                                                 \
      case BlendFunc::MULTIPLY: {                                       \
        using blendfunc_t = pixel_multiply<srctype, dsttype>; /* NOLINT */ \
        expr;                                                           \
        break;                                                          \
      }                                                                 \
      default:                                                          \
        throw std::invalid_argument("unknown blendfunc");               \
    }                                                                   \
  } while (false)

} // namespace surf

#endif

/* EOF */
