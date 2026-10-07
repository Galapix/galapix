// SPDX-FileCopyrightText: 2018 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WSTGUI_FRAME_HUD_HPP
#define HEADER_WSTGUI_FRAME_HUD_HPP

#include "screen.hpp"
#include "style.hpp"

namespace wstgui {

class FrameHud : public Screen
{
public:
  FrameHud(Style& style);

  void draw(wstdisplay::Canvas& canvas) override;
  void update(float delta, const Controller& controller) override;

private:
  Style& m_style;

  float m_time_counter;
  int m_frame_counter;
  int m_last_fps;

private:
  FrameHud(const FrameHud&) = delete;
  FrameHud& operator=(const FrameHud&) = delete;
};

} // namespace wstgui

#endif

/* EOF */
