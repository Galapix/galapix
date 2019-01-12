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

#ifndef HEADER_GALAPIX_GALAPIX_MANDELBROT_TILE_JOB_HPP
#define HEADER_GALAPIX_GALAPIX_MANDELBROT_TILE_JOB_HPP

#include <functional>

#include "job/job.hpp"
#include "math/size.hpp"
#include "math/vector2i.hpp"
#include "galapix/tile.hpp"

class MandelbrotTileJob : public Job
{
private:
  Size     m_size;
  int      m_scale;
  Vector2i m_pos;
  std::function<void (Tile)> m_callback;

public:
  MandelbrotTileJob(JobHandle job_handle, const Size& size, int scale, const Vector2i& pos,
                    const std::function<void (Tile)>& callback);

  void run() override;
};

#endif

/* EOF */
