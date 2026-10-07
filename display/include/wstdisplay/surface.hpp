// SPDX-FileCopyrightText: 2002-2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_SURFACE_HPP
#define HEADER_WSTDISPLAY_SURFACE_HPP

#include <type_traits>

#include <geom/rect.hpp>
#include <geom/size.hpp>

#include "handle.hpp"

namespace wstdisplay {

/** A rectangular region of a texture, the basic unit of drawing.
    Several surfaces can share a texture (atlas pages, sprite sheets),
    which is what allows drawing them in a single batch.

    A small trivially copyable value: copying is free and nothing is
    owned, the texture belongs to whoever created it (SurfaceManager,
    TexturePacker, a Unique<Texture>). */
class Surface final
{
public:
  /** A null surface, draws nothing */
  Surface() = default;

  /** The whole \a texture, which has \a texture_size pixels */
  Surface(TextureId texture, geom::isize const& texture_size);

  /** @param uv    the region on the texture in [0,1] notation
      @param size  the size of the surface in pixels */
  Surface(TextureId texture, geom::frect const& uv, geom::fsize const& size);

  explicit operator bool() const { return static_cast<bool>(m_texture); }

  /** The pixel rectangle \a rect of this surface, sharing the texture */
  Surface region(geom::frect const& rect) const;

  TextureId get_texture() const { return m_texture; }

  /** Texture coordinates of the surface rectangle */
  geom::frect const& get_uv() const { return m_uv; }

  geom::fsize get_size() const { return m_size; }
  float get_width() const { return m_size.width(); }
  float get_height() const { return m_size.height(); }

  friend bool operator==(Surface const& lhs, Surface const& rhs) = default;

private:
  TextureId m_texture = {};
  geom::frect m_uv = {};
  geom::fsize m_size = {};
};

static_assert(std::is_trivially_copyable_v<Surface>);

} // namespace wstdisplay

#endif

/* EOF */
