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

#ifndef HEADER_SURF_PIXEL_FORMAT_HPP
#define HEADER_SURF_PIXEL_FORMAT_HPP

#include <string>

namespace surf {

enum class PixelFormat
{
  NONE,

  RGB8,
  RGBA8,
  RGB16,
  RGBA16,
  RGB32,
  RGBA32,

  RGB32f,
  RGBA32f,
  RGB64f,
  RGBA64f,

  L8,
  LA8,
  L16,
  LA16,
  L32,
  LA32,

  L32f,
  LA32f,
  L64f,
  LA64f,
};

std::string to_string(PixelFormat format);

PixelFormat pixelformat_from_string(std::string_view text);

} // namespace surf

#endif

/* EOF */
