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

#ifndef HEADER_TILE_ENTRY_HPP
#define HEADER_TILE_ENTRY_HPP

#include "math/vector2i.hpp"
#include "software_surface.hpp"

class TileEntry
{
private:
  uint32_t fileid;
  int      scale;
  Vector2i pos;
  Blob     blob;
  SoftwareSurface surface;
  int      format;
  
public:  
  TileEntry()
  {}

  TileEntry(uint32_t fileid_, int scale_, const Vector2i& pos_, const SoftwareSurface& surface_)
    : fileid(fileid_),
      scale(scale_),
      pos(pos_),
      surface(surface_),
      format(-1)
  {}

  TileEntry(uint32_t fileid_, int scale_, const Vector2i& pos_, const Blob& blob_, int format_)
    : fileid(fileid_),
      scale(scale_),
      pos(pos_),
      blob(blob_),
      format(format_)
  {}

  SoftwareSurface get_surface() const { return surface; }
  Blob     get_blob()   const { return blob; }
  uint32_t get_fileid() const { return fileid; }
  int      get_scale()  const { return scale; }
  Vector2i get_pos()    const { return pos; }
  int      get_format() const { return format; }

  void set_fileid(uint32_t fileid_) { fileid = fileid_; }
  void set_surface(const SoftwareSurface& surface_)  { surface = surface_; }
  void set_blob(const Blob& blob_) { blob = blob_; }
  void set_format(int format_) { format = format_; }
};

#endif

/* EOF */
