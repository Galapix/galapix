// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_GALAPIX_TILE_HPP
#define HEADER_GALAPIX_GALAPIX_TILE_HPP

#include <surf/software_surface.hpp>

#include "math/vector2i.hpp"

namespace galapix {

class Tile
{
public:
  Tile() :
    m_scale(),
    m_pos(),
    m_surface(),
    m_valid(false)
  {}

  Tile(int scale, Vector2i const& pos, surf::SoftwareSurface const& surface) :
    m_scale(scale),
    m_pos(pos),
    m_surface(surface),
    m_valid(true)
  {}

  surf::SoftwareSurface get_surface() const { return m_surface; }
  int      get_scale()  const { return m_scale; }
  Vector2i get_pos()    const { return m_pos; }

  explicit operator bool() const
  {
    return m_valid;
  }

private:
  int m_scale;
  Vector2i m_pos;
  surf::SoftwareSurface m_surface;
  bool m_valid;
};

} // namespace galapix

#endif

/* EOF */
