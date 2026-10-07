// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_BLEND_HPP
#define HEADER_WSTDISPLAY_BLEND_HPP

#include <cstdint>
#include <string_view>

namespace wstdisplay {

/** How a drawn pixel is combined with the pixel already in the target */
enum class Blend : uint8_t
{
  /** Overwrite the target, ignore alpha */
  Opaque,

  /** Regular alpha blending with non-premultiplied colors */
  Alpha,

  /** Alpha blending with premultiplied colors */
  Premultiplied,

  /** Add the color weighted by alpha, used for lights and glows */
  Add,

  /** Multiply the target with the color, used for lightmaps */
  Multiply
};

/** Parse "opaque", "alpha", "premultiplied", "add" or "multiply",
    throws on unknown names */
Blend blend_from_string(std::string_view text);
std::string_view to_string(Blend blend);

} // namespace wstdisplay

#endif

/* EOF */
