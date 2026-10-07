// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_TEXTURE_PARAMS_HPP
#define HEADER_WSTDISPLAY_TEXTURE_PARAMS_HPP

#include <cstdint>

namespace wstdisplay {

enum class TextureFilter : uint8_t { Nearest, Linear };

enum class TextureWrap : uint8_t { Clamp, Repeat };

struct TextureParams
{
  TextureFilter filter = TextureFilter::Linear;

  /** Horizontal and vertical wrapping, e.g. repeat only horizontally
      for parallax layers */
  TextureWrap wrap_x = TextureWrap::Clamp;
  TextureWrap wrap_y = TextureWrap::Clamp;

  friend bool operator==(TextureParams const&, TextureParams const&) = default;
};

} // namespace wstdisplay

#endif

/* EOF */
