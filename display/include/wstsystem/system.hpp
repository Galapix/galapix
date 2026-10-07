// SPDX-FileCopyrightText: 2018 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_SYSTEM_HPP
#define HEADER_WSTDISPLAY_SYSTEM_HPP

#include <cstdint>
#include <memory>
#include <unordered_map>

#include <wstsystem/signal.hpp>
#include <geom/rect.hpp>
#include <geom/size.hpp>
#include <wstdisplay/fwd.hpp>
#include <wstdisplay/opengl_window.hpp>

namespace wstsystem {

class System
{
public:
  System();
  ~System();

  std::unique_ptr<wstdisplay::OpenGLWindow> create_window(wstdisplay::OpenGLWindow::Params const& params);
  std::unique_ptr<wstdisplay::OpenGLWindow> create_window(std::string title, geom::isize const& size);

  void run();
  void update();
  int get_ticks();
  void delay(int msec);
  void quit();
  bool is_quit() const { return m_quit; }

  /** Called by OpenGLWindow on destruction */
  void remove_window(uint32_t id);

  wstsystem::Signal<void(SDL_Event const&)>& sig_event() { return m_sig_event; }

public:
  wstsystem::Signal<void(SDL_KeyboardEvent const&)> sig_keyboard_event;
  wstsystem::Signal<void(SDL_TextInputEvent const&)> sig_text_input_event;
  wstsystem::Signal<void(SDL_TextEditingEvent const&)> sig_text_editing_event;
  wstsystem::Signal<void(SDL_MouseButtonEvent const&)> sig_mouse_button_event;
  wstsystem::Signal<void(SDL_MouseMotionEvent const&)> sig_mouse_motion_event;
  wstsystem::Signal<void(SDL_MouseWheelEvent const&)> sig_mouse_wheel_event;

private:
  bool m_quit;
  std::unordered_map<uint32_t, wstdisplay::OpenGLWindow*> m_windows;
  wstsystem::Signal<void(SDL_Event const&)> m_sig_event;

private:
  System(const System&) = delete;
  System& operator=(const System&) = delete;
};

} // namespace wstsystem

#endif

/* EOF */
