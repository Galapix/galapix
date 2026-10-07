// SPDX-FileCopyrightText: 2018 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/frame_hud.hpp>

#include <cmath>
#include <sstream>

#include <wstdisplay/canvas.hpp>

namespace wstgui {

FrameHud::FrameHud(Style& style) :
  m_style(style),
  m_time_counter(0),
  m_frame_counter(0),
  m_last_fps(0)
{
}

void
FrameHud::draw(wstdisplay::Canvas& canvas)
{
  m_frame_counter += 1;

  if (m_time_counter > 1)
  {
    m_last_fps = int(static_cast<float>(m_frame_counter) / m_time_counter);

    m_time_counter  = std::fmod(m_time_counter, 1.0f);
    m_frame_counter = 0;
  }

  if (m_last_fps != 0) {
    std::ostringstream out;
    out << "FPS: " << m_last_fps;
    m_style.get_font()->draw(canvas, geom::fpoint(10.0f, 30.0f), out.str());
  }
}

void
FrameHud::update(float delta, const Controller& controller)
{
  m_time_counter += delta;
}

} // namespace wstgui

/* EOF */
