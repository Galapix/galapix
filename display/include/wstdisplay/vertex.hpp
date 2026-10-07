// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_VERTEX_HPP
#define HEADER_WSTDISPLAY_VERTEX_HPP

#include <array>
#include <bit>
#include <cstdint>
#include <type_traits>

#include <surf/color.hpp>

namespace wstdisplay {

/** A color packed into four bytes in R, G, B, A memory order, the
    layout the GPU expects for normalized unsigned byte attributes. */
using PackedColor = uint32_t;

inline PackedColor pack_color(surf::Color const& color)
{
  return std::bit_cast<PackedColor>(std::array<uint8_t, 4>{
      color.r8(), color.g8(), color.b8(), color.a8()});
}

inline surf::Color unpack_color(PackedColor color)
{
  auto const bytes = std::bit_cast<std::array<uint8_t, 4>>(color);
  return surf::Color::from_rgba8888(bytes[0], bytes[1], bytes[2], bytes[3]);
}

inline constexpr PackedColor packed_white = 0xffffffffu;
inline constexpr PackedColor packed_transparent = 0x00000000u;

/** The single vertex format used for all 2D drawing, 24 bytes.
    Positions are in canvas coordinates with the canvas transform
    already applied. */
struct Vertex
{
  float x = 0.0f;
  float y = 0.0f;
  float u = 0.0f;
  float v = 0.0f;

  /** Multiplied with the texture color */
  PackedColor color = packed_white;

  /** Four freely usable values in [0,1] that reach the shader as
      a_effect, so draws with different parameters still batch. The
      default shader mixes the rgb part into the result by a, e.g.
      white with an alpha of 0.8 for a hit flash, 0 is no effect.
      Custom materials can interpret them as they like (dissolve
      threshold, palette row, ...). */
  PackedColor effect = packed_transparent;
};

static_assert(sizeof(Vertex) == 24);
static_assert(std::is_trivially_copyable_v<Vertex>);

} // namespace wstdisplay

#endif

/* EOF */
