// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "galapix/tile_provider.hpp"

namespace galapix {

void
TileProvider::request_tiles(std::vector<TileRequest> requests)
{
  for (auto& r : requests) {
    request_tile(r.scale, r.pos, r.callback);
  }
}

} // namespace galapix

/* EOF */
