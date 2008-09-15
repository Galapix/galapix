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

#ifndef HEADER_TILE_GENERATOR_HPP
#define HEADER_TILE_GENERATOR_HPP

#include <boost/function.hpp>
#include <string>
#include "software_surface.hpp"
#include "tile_entry.hpp"

class FileEntry;
class URL;

class TileGenerator
{
private:

public:
  TileGenerator();
  ~TileGenerator();

  /** Slow brute force approach to generate tiles, works with all
      image formats */
  void generate_all(int fileid, const SoftwareSurface& surface,
                    const boost::function<void (TileEntry)>& callback);

  void generate_all(int fileid, const URL& url,
                    const boost::function<void (TileEntry)>& callback);

  void generate_quick(const FileEntry& entry,
                      const boost::function<void (TileEntry)>& callback);

private:
  TileGenerator (const TileGenerator&);
  TileGenerator& operator= (const TileGenerator&);
};

#endif

/* EOF */
