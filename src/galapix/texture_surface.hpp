// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_GALAPIX_TEXTURE_SURFACE_HPP
#define HEADER_GALAPIX_GALAPIX_TEXTURE_SURFACE_HPP

#include <memory>

#include <wstdisplay/device.hpp>
#include <wstdisplay/surface.hpp>
#include <wstdisplay/texture.hpp>

namespace galapix {

/** A wstdisplay::Surface together with the texture it is on.
    wstdisplay::Surface is a plain value that owns nothing, Galapix
    shares tiles between the cache and the frames drawing them, so the
    texture is owned here and destroyed with the last reference. All
    TextureSurfaces must be released before the Device goes away. */
class TextureSurface final
{
public:
  TextureSurface(wstdisplay::Unique<wstdisplay::Texture> texture,
                 wstdisplay::Surface const& surface) :
    m_texture(std::move(texture)),
    m_surface(surface)
  {}

  wstdisplay::Surface const& get() const { return m_surface; }

  float get_width() const { return m_surface.get_width(); }
  float get_height() const { return m_surface.get_height(); }

private:
  wstdisplay::Unique<wstdisplay::Texture> m_texture;
  wstdisplay::Surface m_surface;

public:
  TextureSurface(TextureSurface const&) = delete;
  TextureSurface& operator=(TextureSurface const&) = delete;
};

using TextureSurfacePtr = std::shared_ptr<TextureSurface const>;

} // namespace galapix

#endif

/* EOF */
