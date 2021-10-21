// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_DISPLAY_TEXTURE_HPP
#define HEADER_GALAPIX_DISPLAY_TEXTURE_HPP

#include <memory>

#include <geom/fwd.hpp>
#include <surf/software_surface.hpp>
#include <wstdisplay/texture.hpp>

#include "math/rect.hpp"

namespace surf {
class SoftwareSurface;
} // namespace surf

using Texture = wstdisplay::TexturePtr;

#if 0

class Texture
{
public:
  static Texture create(surf::SoftwareSurface const& src, const Rect& srcrect);

public:
  Texture();

  int get_width() const;
  int get_height() const;

  void bind() const;

  explicit operator bool() const { return m_impl != nullptr; }

private:
  Texture(surf::SoftwareSurface const& src, const Rect& srcrect);

private:
  std::shared_ptr<const TextureImpl> m_impl;
};

#endif

#endif

/* EOF */
