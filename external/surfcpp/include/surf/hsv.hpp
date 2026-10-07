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

#ifndef HEADER_SURF_HSV_HPP
#define HEADER_SURF_HSV_HPP

#include <algorithm>
#include <array>
#include <cmath>

#include "color.hpp"

namespace surf {

class HSVColor
{
public:
  float hue = 0.0f;
  float saturation = 0.0f;
  float value = 0.0f;

  bool operator==(HSVColor const& rhs) const = default;
};

inline
HSVColor hsv_from_color(Color const& color)
{
  HSVColor hsvcolor;

  float const& min = std::min(std::min(color.r, color.g), color.b);
  float const& max = std::max(std::max(color.r, color.g), color.b);

  if (min == max) {
    hsvcolor.hue = 0.0f;
  } else {
    if (max == color.r) {
      hsvcolor.hue = (0.0f + (color.g - color.b) / (max - min)) / 6;
    } else if (max == color.g) {
      hsvcolor.hue = (2.0f + (color.b - color.r) / (max - min)) / 6;
    } else if (max == color.b) {
      hsvcolor.hue = (4.0f + (color.r - color.g) / (max - min)) / 6;
    }

    if (hsvcolor.hue < 0.0f) {
      hsvcolor.hue += 1.0f;
    }
  }

  if (max == 0.0f) {
    hsvcolor.saturation = 0.0f;
  } else {
    hsvcolor.saturation = (max - min) / max;
  }

  hsvcolor.value = max;

  return hsvcolor;
}

inline
Color color_from_hue(float hue)
{
  static std::array<Color, 7> colors{
    Color(1.0f, 0.0f, 0.0f),
    Color(1.0f, 1.0f, 0.0f),
    Color(0.0f, 1.0f, 0.0f),
    Color(0.0f, 1.0f, 1.0f),
    Color(0.0f, 0.0f, 1.0f),
    Color(1.0f, 0.0f, 1.0f),
    Color(1.0f, 0.0f, 0.0f),
  };

  // wrap hue into [0, 1)
  hue = hue - std::floor(hue);

  float const seg_len = (1.0f / 6.0f);
  int const seg  = std::clamp(static_cast<int>(hue / seg_len), 0, 5);
  float const prog = std::clamp(hue - static_cast<float>(seg) * seg_len, 0.0f, seg_len);

  return Color(((((seg_len - prog) * colors[seg].r) + (prog * colors[seg + 1].r)) / seg_len),
               ((((seg_len - prog) * colors[seg].g) + (prog * colors[seg + 1].g)) / seg_len),
               ((((seg_len - prog) * colors[seg].b) + (prog * colors[seg + 1].b)) / seg_len));
}

inline
Color apply_saturation_value(Color const& color, float saturation, float value)
{
  return clamp(Color(value * ((1.0f - saturation) + color.r * saturation),
                     value * ((1.0f - saturation) + color.g * saturation),
                     value * ((1.0f - saturation) + color.b * saturation),
                     color.a));
}

inline
Color color_from_hsv(float hue, float saturation, float value)
{
  return apply_saturation_value(color_from_hue(hue), saturation, value);
}

inline
Color color_from_hsv(HSVColor const& hsv)
{
  return apply_saturation_value(color_from_hue(hsv.hue), hsv.saturation, hsv.value);
}

} // namespace surf

#endif

/* EOF */
