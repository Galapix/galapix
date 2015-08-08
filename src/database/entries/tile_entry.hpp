/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef HEADER_GALAPIX_DATABASE_ENTRIES_TILE_ENTRY_HPP
#define HEADER_GALAPIX_DATABASE_ENTRIES_TILE_ENTRY_HPP

#include "math/vector2i.hpp"
#include "util/software_surface.hpp"
#include "database/entries/old_file_entry.hpp"

class TileEntry
{
public:
  enum Format
  {
    UNKNOWN_FORMAT = -1,
    JPEG_FORMAT =  0,
    PNG_FORMAT  =  1
  };

private:
  RowId     m_image_id;
  int        m_scale;
  Vector2i   m_pos;
  Blob m_blob;
  SoftwareSurfacePtr m_surface;
  Format     m_format;

public:
  TileEntry() :
    m_image_id(),
    m_scale(),
    m_pos(),
    m_blob(),
    m_surface(),
    m_format()
  {}

  TileEntry(const RowId& image_id, int scale, const Vector2i& pos, const SoftwareSurfacePtr& surface) :
    m_image_id(image_id),
    m_scale(scale),
    m_pos(pos),
    m_blob(),
    m_surface(surface),
    m_format(UNKNOWN_FORMAT)
  {}

  TileEntry(const RowId& image_id, int scale, const Vector2i& pos, Blob const& blob, Format format) :
    m_image_id(image_id),
    m_scale(scale),
    m_pos(pos),
    m_blob(blob),
    m_surface(),
    m_format(format)
  {}

  SoftwareSurfacePtr get_surface() const { return m_surface; }
  Blob   get_blob()   const { return m_blob; }
  RowId     get_image_id() const { return m_image_id; }
  int       get_scale()  const { return m_scale; }
  Vector2i  get_pos()    const { return m_pos; }
  Format    get_format() const { return m_format; }

  void set_image_id(const RowId& image_id) { m_image_id = image_id; }
  void set_surface(SoftwareSurfacePtr surface)  { m_surface = surface; }
  void set_blob(Blob const& blob) { m_blob = blob; }
  void set_format(Format format) { m_format = format; }

  explicit operator bool() const
  {
    return static_cast<bool>(m_image_id);
  }
};

#endif

/* EOF */
