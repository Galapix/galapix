// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_SCREEN_SCREEN_MANAGER_HPP
#define HEADER_WINDSTILLE_SCREEN_SCREEN_MANAGER_HPP

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include <SDL.h>
#include <wstsystem/signal.hpp>
#include <surf/color.hpp>

#include <wstdisplay/fwd.hpp>
#include <wstgui/fwd.hpp>
#include <wstsystem/system.hpp>
#include <wstinput/fwd.hpp>

namespace wstgui {

/**
 *  The ScreenManager handles overlays like Option Menus, Main Menus
 *  and such
 */
class ScreenManager
{
public:
  ScreenManager(wstsystem::System& system, wstdisplay::OpenGLWindow& window, wstinput::InputManagerSDL& input);
  ~ScreenManager();

  /** Displays the previously set screen in until quit() is called.
      With Emscripten the browser drives the frames and run() doesn't
      return. */
  void run();

  /** Breaks out of the run() function */
  void quit();

  /** Push a screen, the screen will be delete'ed once it is no longer needed */
  void push_screen(std::unique_ptr<Screen> s);
  void pop_screen();

  /**
   *  Push an overlay screen, the screen will be delete'ed once it is
   *  no longer needed. An overlay is a non-fullscreen screen like a
   *  menu that is layed over a fullscreen-screen and receives input.
   */
  void push_overlay(std::unique_ptr<Screen> s);
  void pop_overlay();
  void clear_overlay();

  /** Add a non-interactive overlay */
  void add_hud(Screen* screen);
  void remove_hud(Screen* screen);

  void bind_key(SDL_Keycode code, std::function<void()> callback);

  /** Color the screen is cleared to before drawing */
  void set_clear_color(surf::Color const& color) { m_clear_color = color; }

  wstsystem::Signal<void (float)>& sig_update() { return m_sig_update; };
  wstsystem::Signal<void (wstdisplay::Canvas&)>& sig_draw_begin() { return m_sig_draw_begin; };
  wstsystem::Signal<void (wstdisplay::Canvas&)>& sig_draw_end() { return m_sig_draw_end; };

private:
  /** Update and draw one frame */
  void run_frame();
  void apply_pending_actions();
  void draw();
  void handle_event(const SDL_Event& event);

private:
  enum class ScreenAction { NONE, POP_SCREEN, PUSH_SCREEN, CLEAR_SCREENS };

private:
  wstsystem::System& m_system;
  wstdisplay::OpenGLWindow& m_window;
  wstinput::InputManagerSDL& m_input;

  std::vector<std::shared_ptr<Screen>> m_screens;
  ScreenAction m_screen_action;
  std::shared_ptr<Screen> m_screen_screen;

  std::vector<std::shared_ptr<Screen>>  m_overlay_screens;
  ScreenAction m_overlay_screen_action;
  std::shared_ptr<Screen> m_overlay_screen_screen;

  unsigned int m_ticks;

  float m_overlap_delta;
  bool  m_do_quit;

  std::unordered_map<SDL_Keycode, std::function<void()>> m_key_bindings;
  std::vector<Screen*> m_huds;

  wstsystem::Signal<void (float)> m_sig_update;
  wstsystem::Signal<void (wstdisplay::Canvas&)> m_sig_draw_begin;
  wstsystem::Signal<void (wstdisplay::Canvas&)> m_sig_draw_end;

  std::unique_ptr<wstdisplay::Canvas> m_canvas;
  surf::Color m_clear_color;

public:
  ScreenManager (const ScreenManager&) = delete;
  ScreenManager& operator= (const ScreenManager&) = delete;
};

} // namespace wstgui

#endif

/* EOF */
