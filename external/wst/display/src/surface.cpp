// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/surface.hpp>

namespace wstdisplay {

Surface::Surface(TextureId texture, geom::isize const& texture_size) :
  m_texture(texture),
  m_uv(0.0f, 0.0f, 1.0f, 1.0f),
  m_size(texture_size)
{
}

Surface::Surface(TextureId texture, geom::frect const& uv, geom::fsize const& size) :
  m_texture(texture),
  m_uv(uv),
  m_size(size)
{
}

Surface
Surface::region(geom::frect const& rect) const
{
  float const sx = m_uv.width() / m_size.width();
  float const sy = m_uv.height() / m_size.height();
  geom::frect const uv(m_uv.left() + rect.left() * sx,
                       m_uv.top() + rect.top() * sy,
                       m_uv.left() + rect.right() * sx,
                       m_uv.top() + rect.bottom() * sy);
  return Surface(m_texture, uv, rect.size());
}

} // namespace wstdisplay

/* EOF */
