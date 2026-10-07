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

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <format>

#include "palette.hpp"

namespace surf {
namespace palette {

Color lookup_colorname(std::string_view colorname)
{
  static std::unordered_map<std::string, Color> name2color = {
    { "transparent", transparent },

    // SVG colors
    { "black", black },
    { "silver", silver },
    { "gray", gray },
    { "white", white },
    { "maroon", maroon },
    { "red", red },
    { "purple", purple },
    { "fuchsia", fuchsia },
    { "green", green },
    { "lime", lime },
    { "olive", olive },
    { "yellow", yellow },
    { "navy", navy },
    { "blue", blue },
    { "teal", teal },
    { "aqua", aqua },
    { "orange", orange },
    { "aliceblue", aliceblue },
    { "antiquewhite", antiquewhite },
    { "aquamarine", aquamarine },
    { "azure", azure },
    { "beige", beige },
    { "bisque", bisque },
    { "blanchedalmond", blanchedalmond },
    { "blueviolet", blueviolet },
    { "brown", brown },
    { "burlywood", burlywood },
    { "cadetblue", cadetblue },
    { "chartreuse", chartreuse },
    { "chocolate", chocolate },
    { "coral", coral },
    { "cornflowerblue", cornflowerblue },
    { "cornsilk", cornsilk },
    { "crimson", crimson },
    { "cyan", cyan },
    { "darkblue", darkblue },
    { "darkcyan", darkcyan },
    { "darkgoldenrod", darkgoldenrod },
    { "darkgray", darkgray },
    { "darkgreen", darkgreen },
    { "darkgrey", darkgrey },
    { "darkkhaki", darkkhaki },
    { "darkmagenta", darkmagenta },
    { "darkolivegreen", darkolivegreen },
    { "darkorange", darkorange },
    { "darkorchid", darkorchid },
    { "darkred", darkred },
    { "darksalmon", darksalmon },
    { "darkseagreen", darkseagreen },
    { "darkslateblue", darkslateblue },
    { "darkslategray", darkslategray },
    { "darkslategrey", darkslategrey },
    { "darkturquoise", darkturquoise },
    { "darkviolet", darkviolet },
    { "deeppink", deeppink },
    { "deepskyblue", deepskyblue },
    { "dimgray", dimgray },
    { "dimgrey", dimgrey },
    { "dodgerblue", dodgerblue },
    { "firebrick", firebrick },
    { "floralwhite", floralwhite },
    { "forestgreen", forestgreen },
    { "gainsboro", gainsboro },
    { "ghostwhite", ghostwhite },
    { "gold", gold },
    { "goldenrod", goldenrod },
    { "greenyellow", greenyellow },
    { "grey", grey },
    { "honeydew", honeydew },
    { "hotpink", hotpink },
    { "indianred", indianred },
    { "indigo", indigo },
    { "ivory", ivory },
    { "khaki", khaki },
    { "lavender", lavender },
    { "lavenderblush", lavenderblush },
    { "lawngreen", lawngreen },
    { "lemonchiffon", lemonchiffon },
    { "lightblue", lightblue },
    { "lightcoral", lightcoral },
    { "lightcyan", lightcyan },
    { "lightgoldenrodyellow", lightgoldenrodyellow },
    { "lightgray", lightgray },
    { "lightgreen", lightgreen },
    { "lightgrey", lightgrey },
    { "lightpink", lightpink },
    { "lightsalmon", lightsalmon },
    { "lightseagreen", lightseagreen },
    { "lightskyblue", lightskyblue },
    { "lightslategray", lightslategray },
    { "lightslategrey", lightslategrey },
    { "lightsteelblue", lightsteelblue },
    { "lightyellow", lightyellow },
    { "limegreen", limegreen },
    { "linen", linen },
    { "magenta", magenta },
    { "mediumaquamarine", mediumaquamarine },
    { "mediumblue", mediumblue },
    { "mediumorchid", mediumorchid },
    { "mediumpurple", mediumpurple },
    { "mediumseagreen", mediumseagreen },
    { "mediumslateblue", mediumslateblue },
    { "mediumspringgreen", mediumspringgreen },
    { "mediumturquoise", mediumturquoise },
    { "mediumvioletred", mediumvioletred },
    { "midnightblue", midnightblue },
    { "mintcream", mintcream },
    { "mistyrose", mistyrose },
    { "moccasin", moccasin },
    { "navajowhite", navajowhite },
    { "oldlace", oldlace },
    { "olivedrab", olivedrab },
    { "orangered", orangered },
    { "orchid", orchid },
    { "palegoldenrod", palegoldenrod },
    { "palegreen", palegreen },
    { "paleturquoise", paleturquoise },
    { "palevioletred", palevioletred },
    { "papayawhip", papayawhip },
    { "peachpuff", peachpuff },
    { "peru", peru },
    { "pink", pink },
    { "plum", plum },
    { "powderblue", powderblue },
    { "rosybrown", rosybrown },
    { "royalblue", royalblue },
    { "saddlebrown", saddlebrown },
    { "salmon", salmon },
    { "sandybrown", sandybrown },
    { "seagreen", seagreen },
    { "seashell", seashell },
    { "sienna", sienna },
    { "skyblue", skyblue },
    { "slateblue", slateblue },
    { "slategray", slategray },
    { "slategrey", slategrey },
    { "snow", snow },
    { "springgreen", springgreen },
    { "steelblue", steelblue },
    { "tan", tan },
    { "thistle", thistle },
    { "tomato", tomato },
    { "turquoise", turquoise },
    { "violet", violet },
    { "wheat", wheat },
    { "whitesmoke", whitesmoke },
    { "yellowgreen", yellowgreen },
  };

  std::string colorname_lowercase;
  colorname_lowercase.reserve(colorname.size());
  std::transform(colorname.begin(), colorname.end(),
                 std::back_inserter(colorname_lowercase),
                 [](unsigned char c){ return std::tolower(c); });

  auto it = name2color.find(colorname_lowercase);
  if (it == name2color.end()) {
    throw std::invalid_argument(std::format("unknown color name: {}", colorname));
  } else {
    return it->second;
  }
}

} // namespace palette
} // namespace surf

/* EOF */
