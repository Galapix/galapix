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

#ifndef HEADER_GALAPIX_GALAPIX_TILE_HPP
#define HEADER_GALAPIX_GALAPIX_TILE_HPP

#include "database/entries/tile_entry.hpp"
#include "math/vector2i.hpp"
#include "surface/software_surface.hpp"

class Tile
{
public:
  Tile() :
    m_scale(),
    m_pos(),
    m_surface(),
    m_valid(false)
  {}

  Tile(const TileEntry& tile_entry) :
    m_scale(tile_entry.get_scale()),
    m_pos(tile_entry.get_pos()),
    m_surface(tile_entry.get_surface()),
    m_valid(tile_entry)
  {}

  Tile(int scale, const Vector2i& pos, SoftwareSurface const& surface) :
    m_scale(scale),
    m_pos(pos),
    m_surface(surface),
    m_valid(true)
  {}

  SoftwareSurface get_surface() const { return m_surface; }
  int      get_scale()  const { return m_scale; }
  Vector2i get_pos()    const { return m_pos; }

  explicit operator bool() const
  {
    return m_valid;
  }

private:
  int m_scale;
  Vector2i m_pos;
  SoftwareSurface m_surface;
  bool m_valid;
};

#endif

/* EOF */
