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

#include "galapix/mandelbrot_tile_job.hpp"

#include "math/math.hpp"

#include <cmath>
#include <iostream>

#include <surf/color.hpp>
#include <surf/pixel_data.hpp>

namespace galapix {

using namespace surf;

MandelbrotTileJob::MandelbrotTileJob(JobHandle const& job_handle, Size const& size, int scale, Vector2i const& pos,
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
  SoftwareSurface surface = SoftwareSurface::create(surf::PixelFormat::RGB8, Size(256, 256));

  // Virtual image size at this scale level, in double (int Size overflows near 2^30).
  double const level_w =
    static_cast<double>(m_size.width()) / static_cast<double>(Math::pow2(m_scale));
  double const level_h =
    static_cast<double>(m_size.height()) / static_cast<double>(Math::pow2(m_scale));

  // Deeper tiles (smaller scale) need more iterations before the set "falls apart".
  int const max_iteration = std::min(2000, 80 + 40 * std::max(0, 20 - m_scale));

  for(int py = 0; py < surface.get_height(); ++py)
  {
    if (get_handle().is_aborted()) {
      return;
    }

    for(int px = 0; px < surface.get_width(); ++px)
    {
      double const ix = static_cast<double>(256 * m_pos.x() + px);
      double const iy = static_cast<double>(256 * m_pos.y() + py);
      double const x0 = ix / level_w * 4.0 - 2.5;
      double const y0 = iy / level_h * 3.0 - 1.5;

      double x = 0.0;
      double y = 0.0;

      int iteration = 0;

      while(x*x + y*y <= 4.0 && iteration < max_iteration)
      {
        double const xtemp = x * x - y * y + x0;
        y = 2.0 * x * y + y0;
        x = xtemp;
        ++iteration;
      }

      double const t = static_cast<double>(iteration) / static_cast<double>(max_iteration);
      auto const v = static_cast<uint8_t>(255.0 * t);
      surface.put_pixel({px, py}, Color::from_rgb888(v, v, v));
    }
  }

  m_callback(Tile(m_scale, m_pos, surface));
  get_handle().set_finished();
}

} // namespace galapix

/* EOF */
