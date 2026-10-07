// SPDX-FileCopyrightText: 2000,2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_SCREEN_SCREEN_HPP
#define HEADER_WINDSTILLE_SCREEN_SCREEN_HPP

#include <SDL.h>
#include <wstdisplay/fwd.hpp>
#include <wstinput/fwd.hpp>

namespace wstgui {

using Controller = wstinput::Controller;

class Screen
{
public:
  Screen();
  virtual ~Screen();

  virtual void on_startup() {}

  /** Draw the current screen onto the shared canvas, which is laid
      out in window coordinates */
  virtual void draw(wstdisplay::Canvas& canvas) =0;

  /** Render what doesn't go through the shared canvas, e.g. a
      Compositor or a 3D scene. Called inside the frame after the
      screen has been cleared and before the canvas filled by draw()
      is rendered on top. The frame size is the drawable size, see
      Renderer::get_frame_size(). */
  virtual void render(wstdisplay::Renderer& /*renderer*/) {}

  /** Update the current screen by \a delta and with input from \a
      controller */
  virtual void update(float delta, const Controller& controller) =0;

  /** Called once a new SDL_Event arrives */
  virtual void handle_event(const SDL_Event& ) {}

  /** Marks the screen fro removal */
  virtual void finish() { m_finished = true; }
  virtual bool is_finished() { return m_finished; }

private:
  bool m_finished;
};

} // namespace wstgui

#endif

/* EOF */
