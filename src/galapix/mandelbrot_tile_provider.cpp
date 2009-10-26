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

#include "galapix/mandelbrot_tile_provider.hpp"

#include <iostream>

#include "job/job.hpp"
#include "job/job_manager.hpp"
#include "galapix/mandelbrot_tile_job.hpp"

MandelbrotTileProvider::MandelbrotTileProvider(JobManager& job_manager) :
  m_size(4 * 256 * Math::pow2(15), 3 * 256 * Math::pow2(15)),
  m_max_scale(15),
  m_job_manager(job_manager)
{
}

MandelbrotTileProvider::~MandelbrotTileProvider()
{
}
  
JobHandle
MandelbrotTileProvider::request_tile(int scale, const Vector2i& pos, 
                                     const boost::function<void (Tile)>& callback)
{
  std::cout << "MandelbrotTileProvider::request_tile(): " << scale << " " << pos << std::endl;
  JobHandle job_handle;
  m_job_manager.request(JobPtr(new MandelbrotTileJob(job_handle, m_size, scale, pos, callback)));
  return job_handle;
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

int
MandelbrotTileProvider::get_overlap() const
{
  return 0; 
}

Size
MandelbrotTileProvider::get_size() const
{
  return m_size;
}

/* EOF */
