/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_DATABASE_TILE_ENTRY_HPP
#define HEADER_GALAPIX_DATABASE_TILE_ENTRY_HPP

#include "math/vector2i.hpp"
#include "util/software_surface.hpp"
#include "database/file_entry.hpp"

class TileEntry
{
private:
  FileEntry  m_file_entry;
  int        m_scale;
  Vector2i   m_pos;
  BlobPtr m_blob;
  SoftwareSurfacePtr m_surface;
  int        m_format;
  
public:  
  TileEntry() :
    m_file_entry(),
    m_scale(),
    m_pos(),
    m_blob(),
    m_surface(),
    m_format()
  {}

  TileEntry(const FileEntry& file_entry, int scale, const Vector2i& pos, const SoftwareSurfacePtr& surface) :
    m_file_entry(file_entry),
    m_scale(scale),
    m_pos(pos),
    m_blob(),
    m_surface(surface),
    m_format(-1)
  {}
  
  TileEntry(const FileEntry& file_entry, int scale, const Vector2i& pos, const BlobPtr& blob, int format) :
    m_file_entry(file_entry),
    m_scale(scale),
    m_pos(pos),
    m_blob(blob),
    m_surface(),
    m_format(format)
  {}

  SoftwareSurfacePtr get_surface() const { return m_surface; }
  BlobPtr get_blob()   const { return m_blob; }
  FileEntry get_file_entry() const { return m_file_entry; }
  int      get_scale()  const { return m_scale; }
  Vector2i get_pos()    const { return m_pos; }
  int      get_format() const { return m_format; }

  void set_file_entry(const FileEntry& file_entry) { m_file_entry = file_entry; }
  void set_surface(SoftwareSurfacePtr surface)  { m_surface = surface; }
  void set_blob(const BlobPtr& blob) { m_blob = blob; }
  void set_format(int format) { m_format = format; }

  operator bool() const
  {
    return m_file_entry;
  }
};

#endif

/* EOF */
