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

#ifndef HEADER_GALAPIX_GALAPIX_TILE_PROVIDER_HPP
#define HEADER_GALAPIX_GALAPIX_TILE_PROVIDER_HPP

#include <functional>
#include <memory>
#include <string>

#include "math/size.hpp"

namespace thumtoo::lod {
class TileBackend;
}

namespace galapix {

class TileProvider;

using TileProviderPtr = std::shared_ptr<TileProvider>;

/** A TileProvider is the data source of an Image: its size, scale range
    and a thumtoo::lod::TileBackend that produces the tiles (see
    ImageTiles and thumtoo docs/TILE_LOD.md) */
class TileProvider
{
public:
  TileProvider() {}
  virtual ~TileProvider() {}

  /** Tile producer for this image. Must answer every requested cell
      exactly once (Ok, Cancelled, Failed or Unavailable). */
  virtual std::shared_ptr<thumtoo::lod::TileBackend> create_backend() =0;

  /** Images with the same non-empty key share one tile loader (one
      request per cell, one copy of the pixels). Empty: not shared. */
  virtual std::string get_loader_key() const { return {}; }

  virtual int get_max_scale() const =0;

  /** Finest scale the provider can produce. Raster images: 0. PDF/live:
      may be negative (sharper than nominal get_size() layout). */
  virtual int get_min_scale() const { return 0; }

  virtual int get_tilesize() const =0;
  virtual Size get_size() const =0;

  virtual void refresh(const std::function<void (TileProviderPtr)>& callback) {}

private:
  TileProvider(TileProvider const&);
  TileProvider& operator=(TileProvider const&);
};

} // namespace galapix

#endif

/* EOF */
