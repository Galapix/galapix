// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_MATH_VECTOR2_HPP
#define HEADER_GALAPIX_MATH_VECTOR2_HPP

#include <iosfwd>
#include <sstream>
#include <fmt/format.h>

#define GLM_FORCE_CTOR_INIT
#include <glm/glm.hpp>

typedef glm::vec2 Vector2f;

std::ostream& operator<<(std::ostream& s, const Vector2f& v);

template<>
struct fmt::formatter<Vector2f>
{
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template<typename FormatContext>
  auto format(Vector2f const& v, FormatContext& ctx)
  {
    std::ostringstream os;
    os << v;
    return fmt::format_to(ctx.out(), os.str());
  }
};

#endif

/* EOF */

