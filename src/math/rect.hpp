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

#ifndef __MATH_RECT_HPP__
#define __MATH_RECT_HPP__

#include <math.h>
#include <fmt/format.h>

#include <geom/geom.hpp>
#include "math/origin.hpp"
#include "math/vector2f.hpp"
#include "math/vector2i.hpp"
#include "math/size.hpp"
#include "math/math.hpp"

using Rectf = geom::frect;
using Rect = geom::irect;

template<>
struct fmt::formatter<Rect>
{
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template<typename FormatContext>
  auto format(Rect const& v, FormatContext& ctx)
  {
    std::ostringstream os;
    os << v;
    return fmt::format_to(ctx.out(), os.str());
  }
};

template<>
struct fmt::formatter<Rectf>
{
  template<typename ParseContext>
  constexpr auto parse(ParseContext& ctx)
  {
    return ctx.begin();
  }

  template<typename FormatContext>
  auto format(Rectf const& v, FormatContext& ctx)
  {
    std::ostringstream os;
    os << v;
    return fmt::format_to(ctx.out(), os.str());
  }
};

#endif

