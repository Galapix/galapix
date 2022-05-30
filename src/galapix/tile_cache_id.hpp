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

#ifndef HEADER_GALAPIX_GALAPIX_TILE_CACHE_ID_HPP
#define HEADER_GALAPIX_GALAPIX_TILE_CACHE_ID_HPP

class TileCacheId
{
public:
  TileCacheId(Vector2i const& pos, int scale) :
    m_pos(pos),
    m_scale(scale)
  {}

  int get_scale() const { return m_scale; }
  Vector2i get_pos() const { return m_pos; }

  bool operator<(TileCacheId const& rhs) const
  {
    if (m_scale < rhs.m_scale)
    {
      return true;
    }
    else if (m_scale > rhs.m_scale)
    {
      return false;
    }
    else
    {
      if (m_pos.x() < rhs.m_pos.x())
      {
        return true;
      }
      else if (m_pos.x() > rhs.m_pos.x())
      {
        return false;
      }
      else
      {
        return m_pos.y() < rhs.m_pos.y();
      }
    }
  }

private:
  Vector2i m_pos;
  int m_scale;
};

#endif

/* EOF */
