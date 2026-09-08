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

#ifndef HEADER_GALAPIX_GALAPIX_OPTIONS_HPP
#define HEADER_GALAPIX_GALAPIX_OPTIONS_HPP

#include <vector>
#include <string>

#include "math/size.hpp"

namespace galapix {

class Options
{
public:
  Options() :
    fullscreen(false),
    geometry(1024, 768),
    anti_aliasing(0),

    database(),
    patterns(),
    threads(),
    rest(),
#ifdef HAVE_THUMTOO
    // Prefer thumtoo tiles when the binary was built with the backend.
    use_thumtoo(true),
#else
    use_thumtoo(false),
#endif
    thumtoo_cache()
  {}

public:
  bool fullscreen;
  Size geometry;
  int  anti_aliasing;

  std::string database;
  std::vector<std::string> patterns;
  int         threads;
  std::vector<std::string> rest;

  /** When true, use ThumtooTileProvider for files.
      HAVE_THUMTOO builds force true (--no-thumtoo is an error). Without
      HAVE_THUMTOO, plain files have no tile provider (SQLite tiles removed). */
  bool use_thumtoo;
  /** thumtoo cache root; empty → $XDG_CACHE_HOME/thumtoo or ~/.cache/thumtoo */
  std::string thumtoo_cache;
};

} // namespace galapix

#endif

/* EOF */
