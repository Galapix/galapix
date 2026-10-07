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

#ifndef HEADER_SURF_COLOR_HPP
#define HEADER_SURF_COLOR_HPP

#include <algorithm>
#include <cstdint>
#include <string_view>

namespace surf {

class Color
{
public:
  using value_type = float;
  static constexpr bool has_alpha() { return true; }
  static constexpr bool has_rgb() { return true; }
  static constexpr float max() { return 1.0f; }

  static constexpr Color from_rgba8888(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    return Color(static_cast<float>(r) / 255.0f,
                 static_cast<float>(g) / 255.0f,
                 static_cast<float>(b) / 255.0f,
                 static_cast<float>(a) / 255.0f);
  }

  static constexpr Color from_rgb888(uint8_t r, uint8_t g, uint8_t b) {
    return Color(static_cast<float>(r) / 255.0f,
                 static_cast<float>(g) / 255.0f,
                 static_cast<float>(b) / 255.0f,
                 1.0f);
  }

  static Color from_string(std::string_view text);

public:
  constexpr Color() :
    r(0.0f), g(0.0f), b(0.0f), a(0.0f)
  {}

  constexpr Color(float r_, float g_, float b_, float a_ = 1.0f) :
    r(r_), g(g_), b(b_), a(a_)
  {}

  inline constexpr uint8_t r8() const { return static_cast<uint8_t>(std::clamp(255.0f * r, 0.0f, 255.0f) + 0.5f); }
  inline constexpr uint8_t g8() const { return static_cast<uint8_t>(std::clamp(255.0f * g, 0.0f, 255.0f) + 0.5f); }
  inline constexpr uint8_t b8() const { return static_cast<uint8_t>(std::clamp(255.0f * b, 0.0f, 255.0f) + 0.5f); }
  inline constexpr uint8_t a8() const { return static_cast<uint8_t>(std::clamp(255.0f * a, 0.0f, 255.0f) + 0.5f); }

  bool operator==(Color const& rhs) const = default;

public:
  float r;
  float g;
  float b;
  float a;
};

/** clamp each color component individually */
inline
Color clamp(Color const& color)
{
  return Color(std::clamp(color.r, 0.0f, 1.0f),
               std::clamp(color.g, 0.0f, 1.0f),
               std::clamp(color.b, 0.0f, 1.0f),
               std::clamp(color.a, 0.0f, 1.0f));
}

/** clamp the color vector, preserving it's angle */
inline
Color boxclamp(Color const& color)
{
  if (color.r <= 1.0f && color.g <= 1.0f && color.b <= 1.0f) {
    return color;
  }

  if (color.r >= color.g && color.r >= color.b) {
    return Color(color.r / color.r,
                 color.g / color.r,
                 color.b / color.r,
                 color.a);
  } else if (color.g >= color.r && color.g >= color.b) {
    return Color(color.r / color.g,
                 color.g / color.g,
                 color.b / color.g,
                 color.a);
  } else { // if (color.b >= color.r && color.r >= color.g) {
    return Color(color.r / color.b,
                 color.g / color.b,
                 color.b / color.b,
                 color.a);
  }
}

} // namespace surf

#endif

/* EOF */
