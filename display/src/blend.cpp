// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/blend.hpp>

#include <stdexcept>
#include <string>

namespace wstdisplay {

Blend blend_from_string(std::string_view text)
{
  if (text == "opaque") { return Blend::Opaque; }
  if (text == "alpha") { return Blend::Alpha; }
  if (text == "premultiplied") { return Blend::Premultiplied; }
  if (text == "add") { return Blend::Add; }
  if (text == "multiply") { return Blend::Multiply; }
  throw std::invalid_argument("unknown blend mode: " + std::string(text));
}

std::string_view to_string(Blend blend)
{
  switch (blend)
  {
    case Blend::Opaque: return "opaque";
    case Blend::Alpha: return "alpha";
    case Blend::Premultiplied: return "premultiplied";
    case Blend::Add: return "add";
    case Blend::Multiply: return "multiply";
  }
  return "alpha";
}

} // namespace wstdisplay

/* EOF */
