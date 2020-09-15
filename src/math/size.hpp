/*
**  ClanLib SDK
**  Copyright (c) 1997-2005 The ClanLib Team
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
**  Note: Some of the libraries ClanLib may link to may have additional
**  requirements or restrictions.
**
**  File Author(s):
**
**    Magnus Norddahl
**    (if your name is missing here, please add it)
*/

//! clanCore="Math"
//! header=core.h

#ifndef HEADER_GALAPIX_MATH_SIZE_HPP
#define HEADER_GALAPIX_MATH_SIZE_HPP

#include <iosfwd>
#include <sstream>
#include <fmt/format.h>
#include <geom/size.hpp>

using Sizef = geom::fsize;
using Size = geom::isize;

std::ostream& operator<<(std::ostream& s, const Size& size);

template<>
struct fmt::formatter<Size>
{
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template<typename FormatContext>
  auto format(Size const& v, FormatContext& ctx)
  {
    std::ostringstream os;
    os << v;
    return fmt::format_to(ctx.out(), os.str());
  }
};

#endif
