// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_APP_IMGUI_OVERLAY_HPP
#define HEADER_GALAPIX_APP_IMGUI_OVERLAY_HPP

#include <SDL.h>
#include <array>
#include <chrono>
#include <deque>
#include <string>

namespace galapix {

class Viewer;

/** Dear ImGui chrome (toolbar + optional status/help) on the SDL OpenGL viewer.
 *
 *  Toggle all chrome with Tab or F1. Status and Help panels are hidden by
 *  default and each has its own toolbar button.
 */
class ImguiOverlay
{
public:
  ImguiOverlay();
  ~ImguiOverlay();

  bool init(SDL_Window* window, SDL_GLContext gl_context);
  void shutdown();

  bool visible() const { return m_visible; }
  void set_visible(bool v) { m_visible = v; }
  void toggle() { m_visible = !m_visible; }

  /** Feed SDL events. Returns true if ImGui wants to capture this event. */
  bool process_event(SDL_Event const& event);

  void begin_frame();
  void draw_status(Viewer& viewer);
  void end_frame();

  bool want_capture_mouse() const;
  bool want_capture_keyboard() const;

  /** Queue a short-lived on-screen toast (also still printed to stdout by callers).
      Safe from the main/GUI thread only. Visible even when Tab/F1 chrome is hidden. */
  static void notify(std::string message, float duration_seconds = 3.0f);

private:
  struct Icon
  {
    unsigned id = 0; // GLuint
    int w = 0;
    int h = 0;
  };

  enum class IconId
  {
    Pan = 0,
    ZoomRect,
    GridTool,
    Move,
    Grid,
    GridPin,
    LayoutRegular,
    LayoutTight,
    LayoutRandom,
    LayoutSolve,
    LayoutSpiral,
    LayoutVertical,
    Status,
    Help,
    Count
  };

  bool load_icons();
  void destroy_icons();
  static std::string data_root();
  bool load_icon_png(IconId id, std::string const& filename);

  /** Toolbar ImageButton (or letter fallback) that toggles a panel flag. */
  void panel_toggle_btn(IconId id, char const* id_str, char const* tip_on,
                        char const* tip_off, bool& visible_flag);

  void draw_status_panel(Viewer& viewer);
  void draw_help_panel(Viewer& viewer);
  void draw_toasts();

  struct Toast
  {
    std::string text;
    std::chrono::steady_clock::time_point expires;
  };

  bool m_initialized = false;
  bool m_visible = true;
  bool m_status_visible = false; // runtime stats; toolbar toggle
  bool m_help_visible = false;   // keyboard shortcuts; toolbar toggle
  bool m_icons_loaded = false;
  std::array<Icon, static_cast<size_t>(IconId::Count)> m_icons{};
  std::deque<Toast> m_toasts;

  static ImguiOverlay* s_instance;
  static constexpr size_t kMaxToasts = 8;
};

} // namespace galapix

#endif

/* EOF */
