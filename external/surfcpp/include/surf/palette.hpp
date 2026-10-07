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

#ifndef HEADER_SURF_PALETTE_HPP
#define HEADER_SURF_PALETTE_HPP

#include <string_view>

#include "color.hpp"

namespace surf {
namespace palette {

constexpr Color transparent = Color(0.0f, 0.0f, 0.0f, 0.0f);

// https://www.w3.org/TR/css-color-3/#svg-color

constexpr Color aliceblue = Color::from_rgb888(0xf0, 0xf8, 0xff);
constexpr Color antiquewhite = Color::from_rgb888(0xfa, 0xeb, 0xd7);
constexpr Color aqua = Color::from_rgb888(0x00, 0xff, 0xff);
constexpr Color aquamarine = Color::from_rgb888(0x7f, 0xff, 0xd4);
constexpr Color azure = Color::from_rgb888(0xf0, 0xff, 0xff);
constexpr Color beige = Color::from_rgb888(0xf5, 0xf5, 0xdc);
constexpr Color bisque = Color::from_rgb888(0xff, 0xe4, 0xc4);
constexpr Color black = Color::from_rgb888(0x00, 0x00, 0x00);
constexpr Color blanchedalmond = Color::from_rgb888(0xff, 0xeb, 0xcd);
constexpr Color blue = Color::from_rgb888(0x00, 0x00, 0xff);
constexpr Color blueviolet = Color::from_rgb888(0x8a, 0x2b, 0xe2);
constexpr Color brown = Color::from_rgb888(0xa5, 0x2a, 0x2a);
constexpr Color burlywood = Color::from_rgb888(0xde, 0xb8, 0x87);
constexpr Color cadetblue = Color::from_rgb888(0x5f, 0x9e, 0xa0);
constexpr Color chartreuse = Color::from_rgb888(0x7f, 0xff, 0x00);
constexpr Color chocolate = Color::from_rgb888(0xd2, 0x69, 0x1e);
constexpr Color coral = Color::from_rgb888(0xff, 0x7f, 0x50);
constexpr Color cornflowerblue = Color::from_rgb888(0x64, 0x95, 0xed);
constexpr Color cornsilk = Color::from_rgb888(0xff, 0xf8, 0xdc);
constexpr Color crimson = Color::from_rgb888(0xdc, 0x14, 0x3c);
constexpr Color cyan = Color::from_rgb888(0x00, 0xff, 0xff);
constexpr Color darkblue = Color::from_rgb888(0x00, 0x00, 0x8b);
constexpr Color darkcyan = Color::from_rgb888(0x00, 0x8b, 0x8b);
constexpr Color darkgoldenrod = Color::from_rgb888(0xb8, 0x86, 0x0b);
constexpr Color darkgray = Color::from_rgb888(0xa9, 0xa9, 0xa9);
constexpr Color darkgreen = Color::from_rgb888(0x00, 0x64, 0x00);
constexpr Color darkgrey = Color::from_rgb888(0xa9, 0xa9, 0xa9);
constexpr Color darkkhaki = Color::from_rgb888(0xbd, 0xb7, 0x6b);
constexpr Color darkmagenta = Color::from_rgb888(0x8b, 0x00, 0x8b);
constexpr Color darkolivegreen = Color::from_rgb888(0x55, 0x6b, 0x2f);
constexpr Color darkorange = Color::from_rgb888(0xff, 0x8c, 0x00);
constexpr Color darkorchid = Color::from_rgb888(0x99, 0x32, 0xcc);
constexpr Color darkred = Color::from_rgb888(0x8b, 0x00, 0x00);
constexpr Color darksalmon = Color::from_rgb888(0xe9, 0x96, 0x7a);
constexpr Color darkseagreen = Color::from_rgb888(0x8f, 0xbc, 0x8f);
constexpr Color darkslateblue = Color::from_rgb888(0x48, 0x3d, 0x8b);
constexpr Color darkslategray = Color::from_rgb888(0x2f, 0x4f, 0x4f);
constexpr Color darkslategrey = Color::from_rgb888(0x2f, 0x4f, 0x4f);
constexpr Color darkturquoise = Color::from_rgb888(0x00, 0xce, 0xd1);
constexpr Color darkviolet = Color::from_rgb888(0x94, 0x00, 0xd3);
constexpr Color deeppink = Color::from_rgb888(0xff, 0x14, 0x93);
constexpr Color deepskyblue = Color::from_rgb888(0x00, 0xbf, 0xff);
constexpr Color dimgray = Color::from_rgb888(0x69, 0x69, 0x69);
constexpr Color dimgrey = Color::from_rgb888(0x69, 0x69, 0x69);
constexpr Color dodgerblue = Color::from_rgb888(0x1e, 0x90, 0xff);
constexpr Color firebrick = Color::from_rgb888(0xb2, 0x22, 0x22);
constexpr Color floralwhite = Color::from_rgb888(0xff, 0xfa, 0xf0);
constexpr Color forestgreen = Color::from_rgb888(0x22, 0x8b, 0x22);
constexpr Color fuchsia = Color::from_rgb888(0xff, 0x00, 0xff);
constexpr Color gainsboro = Color::from_rgb888(0xdc, 0xdc, 0xdc);
constexpr Color ghostwhite = Color::from_rgb888(0xf8, 0xf8, 0xff);
constexpr Color gold = Color::from_rgb888(0xff, 0xd7, 0x00);
constexpr Color goldenrod = Color::from_rgb888(0xda, 0xa5, 0x20);
constexpr Color gray = Color::from_rgb888(0x80, 0x80, 0x80);
constexpr Color green = Color::from_rgb888(0x00, 0x80, 0x00);
constexpr Color greenyellow = Color::from_rgb888(0xad, 0xff, 0x2f);
constexpr Color grey = Color::from_rgb888(0x80, 0x80, 0x80);
constexpr Color honeydew = Color::from_rgb888(0xf0, 0xff, 0xf0);
constexpr Color hotpink = Color::from_rgb888(0xff, 0x69, 0xb4);
constexpr Color indianred = Color::from_rgb888(0xcd, 0x5c, 0x5c);
constexpr Color indigo = Color::from_rgb888(0x4b, 0x00, 0x82);
constexpr Color ivory = Color::from_rgb888(0xff, 0xff, 0xf0);
constexpr Color khaki = Color::from_rgb888(0xf0, 0xe6, 0x8c);
constexpr Color lavender = Color::from_rgb888(0xe6, 0xe6, 0xfa);
constexpr Color lavenderblush = Color::from_rgb888(0xff, 0xf0, 0xf5);
constexpr Color lawngreen = Color::from_rgb888(0x7c, 0xfc, 0x00);
constexpr Color lemonchiffon = Color::from_rgb888(0xff, 0xfa, 0xcd);
constexpr Color lightblue = Color::from_rgb888(0xad, 0xd8, 0xe6);
constexpr Color lightcoral = Color::from_rgb888(0xf0, 0x80, 0x80);
constexpr Color lightcyan = Color::from_rgb888(0xe0, 0xff, 0xff);
constexpr Color lightgoldenrodyellow = Color::from_rgb888(0xfa, 0xfa, 0xd2);
constexpr Color lightgray = Color::from_rgb888(0xd3, 0xd3, 0xd3);
constexpr Color lightgreen = Color::from_rgb888(0x90, 0xee, 0x90);
constexpr Color lightgrey = Color::from_rgb888(0xd3, 0xd3, 0xd3);
constexpr Color lightpink = Color::from_rgb888(0xff, 0xb6, 0xc1);
constexpr Color lightsalmon = Color::from_rgb888(0xff, 0xa0, 0x7a);
constexpr Color lightseagreen = Color::from_rgb888(0x20, 0xb2, 0xaa);
constexpr Color lightskyblue = Color::from_rgb888(0x87, 0xce, 0xfa);
constexpr Color lightslategray = Color::from_rgb888(0x77, 0x88, 0x99);
constexpr Color lightslategrey = Color::from_rgb888(0x77, 0x88, 0x99);
constexpr Color lightsteelblue = Color::from_rgb888(0xb0, 0xc4, 0xde);
constexpr Color lightyellow = Color::from_rgb888(0xff, 0xff, 0xe0);
constexpr Color lime = Color::from_rgb888(0x00, 0xff, 0x00);
constexpr Color limegreen = Color::from_rgb888(0x32, 0xcd, 0x32);
constexpr Color linen = Color::from_rgb888(0xfa, 0xf0, 0xe6);
constexpr Color magenta = Color::from_rgb888(0xff, 0x00, 0xff);
constexpr Color maroon = Color::from_rgb888(0x80, 0x00, 0x00);
constexpr Color mediumaquamarine = Color::from_rgb888(0x66, 0xcd, 0xaa);
constexpr Color mediumblue = Color::from_rgb888(0x00, 0x00, 0xcd);
constexpr Color mediumorchid = Color::from_rgb888(0xba, 0x55, 0xd3);
constexpr Color mediumpurple = Color::from_rgb888(0x93, 0x70, 0xdb);
constexpr Color mediumseagreen = Color::from_rgb888(0x3c, 0xb3, 0x71);
constexpr Color mediumslateblue = Color::from_rgb888(0x7b, 0x68, 0xee);
constexpr Color mediumspringgreen = Color::from_rgb888(0x00, 0xfa, 0x9a);
constexpr Color mediumturquoise = Color::from_rgb888(0x48, 0xd1, 0xcc);
constexpr Color mediumvioletred = Color::from_rgb888(0xc7, 0x15, 0x85);
constexpr Color midnightblue = Color::from_rgb888(0x19, 0x19, 0x70);
constexpr Color mintcream = Color::from_rgb888(0xf5, 0xff, 0xfa);
constexpr Color mistyrose = Color::from_rgb888(0xff, 0xe4, 0xe1);
constexpr Color moccasin = Color::from_rgb888(0xff, 0xe4, 0xb5);
constexpr Color navajowhite = Color::from_rgb888(0xff, 0xde, 0xad);
constexpr Color navy = Color::from_rgb888(0x00, 0x00, 0x80);
constexpr Color oldlace = Color::from_rgb888(0xfd, 0xf5, 0xe6);
constexpr Color olive = Color::from_rgb888(0x80, 0x80, 0x00);
constexpr Color olivedrab = Color::from_rgb888(0x6b, 0x8e, 0x23);
constexpr Color orange = Color::from_rgb888(0xff, 0xa5, 0x00);
constexpr Color orangered = Color::from_rgb888(0xff, 0x45, 0x00);
constexpr Color orchid = Color::from_rgb888(0xda, 0x70, 0xd6);
constexpr Color palegoldenrod = Color::from_rgb888(0xee, 0xe8, 0xaa);
constexpr Color palegreen = Color::from_rgb888(0x98, 0xfb, 0x98);
constexpr Color paleturquoise = Color::from_rgb888(0xaf, 0xee, 0xee);
constexpr Color palevioletred = Color::from_rgb888(0xdb, 0x70, 0x93);
constexpr Color papayawhip = Color::from_rgb888(0xff, 0xef, 0xd5);
constexpr Color peachpuff = Color::from_rgb888(0xff, 0xda, 0xb9);
constexpr Color peru = Color::from_rgb888(0xcd, 0x85, 0x3f);
constexpr Color pink = Color::from_rgb888(0xff, 0xc0, 0xcb);
constexpr Color plum = Color::from_rgb888(0xdd, 0xa0, 0xdd);
constexpr Color powderblue = Color::from_rgb888(0xb0, 0xe0, 0xe6);
constexpr Color purple = Color::from_rgb888(0x80, 0x00, 0x80);
constexpr Color red = Color::from_rgb888(0xff, 0x00, 0x00);
constexpr Color rosybrown = Color::from_rgb888(0xbc, 0x8f, 0x8f);
constexpr Color royalblue = Color::from_rgb888(0x41, 0x69, 0xe1);
constexpr Color saddlebrown = Color::from_rgb888(0x8b, 0x45, 0x13);
constexpr Color salmon = Color::from_rgb888(0xfa, 0x80, 0x72);
constexpr Color sandybrown = Color::from_rgb888(0xf4, 0xa4, 0x60);
constexpr Color seagreen = Color::from_rgb888(0x2e, 0x8b, 0x57);
constexpr Color seashell = Color::from_rgb888(0xff, 0xf5, 0xee);
constexpr Color sienna = Color::from_rgb888(0xa0, 0x52, 0x2d);
constexpr Color silver = Color::from_rgb888(0xc0, 0xc0, 0xc0);
constexpr Color skyblue = Color::from_rgb888(0x87, 0xce, 0xeb);
constexpr Color slateblue = Color::from_rgb888(0x6a, 0x5a, 0xcd);
constexpr Color slategray = Color::from_rgb888(0x70, 0x80, 0x90);
constexpr Color slategrey = Color::from_rgb888(0x70, 0x80, 0x90);
constexpr Color snow = Color::from_rgb888(0xff, 0xfa, 0xfa);
constexpr Color springgreen = Color::from_rgb888(0x00, 0xff, 0x7f);
constexpr Color steelblue = Color::from_rgb888(0x46, 0x82, 0xb4);
constexpr Color tan = Color::from_rgb888(0xd2, 0xb4, 0x8c);
constexpr Color teal = Color::from_rgb888(0x00, 0x80, 0x80);
constexpr Color thistle = Color::from_rgb888(0xd8, 0xbf, 0xd8);
constexpr Color tomato = Color::from_rgb888(0xff, 0x63, 0x47);
constexpr Color turquoise = Color::from_rgb888(0x40, 0xe0, 0xd0);
constexpr Color violet = Color::from_rgb888(0xee, 0x82, 0xee);
constexpr Color wheat = Color::from_rgb888(0xf5, 0xde, 0xb3);
constexpr Color white = Color::from_rgb888(0xff, 0xff, 0xff);
constexpr Color whitesmoke = Color::from_rgb888(0xf5, 0xf5, 0xf5);
constexpr Color yellow = Color::from_rgb888(0xff, 0xff, 0x00);
constexpr Color yellowgreen = Color::from_rgb888(0x9a, 0xcd, 0x32);

Color lookup_colorname(std::string_view colorname);

} // namespace palette
} // namespace surf

#endif

/* EOF */
