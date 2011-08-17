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

#ifndef HEADER_GALAPIX_GALAPIX_TILE_PROVIDER_HPP
#define HEADER_GALAPIX_GALAPIX_TILE_PROVIDER_HPP

#include <memory>
#include <boost/function.hpp>

#include "galapix/tile.hpp"
#include "job/job_handle.hpp"

class TileProvider;

typedef std::shared_ptr<TileProvider> TileProviderPtr;

class TileProvider
{
private:
public:
  TileProvider() {}
  virtual ~TileProvider() {}
  
  virtual JobHandle request_tile(int tilescale, const Vector2i& pos, 
                                 const std::function<void (Tile)>& callback) =0;

  virtual int  get_max_scale() const =0;
  virtual int  get_tilesize() const =0;
  virtual int  get_overlap() const =0;
  virtual Size get_size() const =0;

  virtual void refresh(const std::function<void (TileProviderPtr)>& callback) {}

private:
  TileProvider(const TileProvider&);
  TileProvider& operator=(const TileProvider&);
};

#endif

/* EOF */
