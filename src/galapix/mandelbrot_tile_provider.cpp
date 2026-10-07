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

#include "galapix/mandelbrot_tile_provider.hpp"

#include <iostream>

#include "job/job.hpp"
#include "job/job_manager.hpp"
#include "galapix/job_tile_backend.hpp"
#include "galapix/mandelbrot_tile_job.hpp"

#include "math/math.hpp"

namespace galapix {

// 20 is the maximum value allowed on 32bit, larger values will make
// the m_size field overflow
const int c_max_scale = 20;

MandelbrotTileProvider::MandelbrotTileProvider(JobManager& job_manager) :
  m_size(4 * 256 * Math::pow2(c_max_scale),
         3 * 256 * Math::pow2(c_max_scale)),
  m_max_scale(c_max_scale),
  m_job_manager(job_manager)
{
}

MandelbrotTileProvider::~MandelbrotTileProvider()
{
}

std::shared_ptr<thumtoo::lod::TileBackend>
MandelbrotTileProvider::create_backend()
{
  Size const size = m_size;
  return std::make_shared<JobTileBackend>(
    m_job_manager, 0, m_max_scale,
    [size](JobHandle const& handle, int scale, Vector2i const& pos,
           std::function<void (Tile)> const& callback) -> JobPtr {
      return std::make_shared<MandelbrotTileJob>(handle, size, scale, pos, callback);
    });
}

int
MandelbrotTileProvider::get_max_scale() const
{
  return m_max_scale;
}

int
MandelbrotTileProvider::get_tilesize() const
{
  return 256;
}

Size
MandelbrotTileProvider::get_size() const
{
  return m_size;
}

} // namespace galapix

/* EOF */
