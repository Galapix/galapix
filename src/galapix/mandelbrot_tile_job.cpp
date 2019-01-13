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

#include "galapix/mandelbrot_tile_job.hpp"

#include <iostream>

#include "math/rgb.hpp"
#include "util/pixel_data.hpp"

MandelbrotTileJob::MandelbrotTileJob(JobHandle const& job_handle, const Size& size, int scale, const Vector2i& pos,
                                     const std::function<void (Tile)>& callback) :
  Job(job_handle),
  m_size(size),
  m_scale(scale),
  m_pos(pos),
  m_callback(callback)
{}

void
MandelbrotTileJob::run()
{
  PixelData surface(PixelData::RGB_FORMAT, Size(256, 256));

  Size imagesize(m_size.width  / Math::pow2(m_scale),
                 m_size.height / Math::pow2(m_scale));

  for(int py = 0; py < surface.get_height(); ++py)
  {
    if (get_handle().is_aborted())
      return;

    for(int px = 0; px < surface.get_width(); ++px)
    {
      double x0 = static_cast<double>(256 * m_pos.x + px) / static_cast<double>(imagesize.width)  * 4.0 - 2.5;
      double y0 = static_cast<double>(256 * m_pos.y + py) / static_cast<double>(imagesize.height) * 3.0 - 1.5;

      double x = 0;
      double y = 0;

      int iteration = 0;
      const int max_iteration = 160;

      while(x*x + y*y <= (2*2) &&
            iteration < max_iteration)
      {
        double xtemp = x * x - y * y + x0;
        y = 2 * x * y + y0;

        x = xtemp;

        ++iteration;
      }

      surface.put_pixel(px, py, RGB(static_cast<uint8_t>(255 * iteration / max_iteration),
                                    static_cast<uint8_t>(255 * iteration / max_iteration),
                                    static_cast<uint8_t>(255 * iteration / max_iteration)));
    }
  }

  m_callback(Tile(m_scale, m_pos, SoftwareSurface(surface)));
  get_handle().set_finished();
}

/* EOF */
