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

#ifndef HEADER_GALAPIX_GALAPIX_MANDELBROT_TILE_PROVIDER_HPP
#define HEADER_GALAPIX_GALAPIX_MANDELBROT_TILE_PROVIDER_HPP

#include <memory>
#include <functional>

#include "galapix/tile_provider.hpp"
#include "galapix/tile.hpp"
#include "job/job_handle.hpp"

class JobManager;

class MandelbrotTileProvider : public TileProvider
{
public:
  MandelbrotTileProvider(JobManager& job_manager);
  ~MandelbrotTileProvider() override;

  JobHandle request_tile(int tilescale, const Vector2i& pos,
                                 const std::function<void (Tile)>& callback) override;

  int  get_max_scale() const override;
  int  get_tilesize() const override;
  Size get_size() const override;

private:
  Size m_size;
  int  m_max_scale;
  JobManager& m_job_manager;

private:
  MandelbrotTileProvider(const MandelbrotTileProvider&);
  MandelbrotTileProvider& operator=(const MandelbrotTileProvider&);
};

#endif

/* EOF */
