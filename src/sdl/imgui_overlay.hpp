// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_SDL_IMGUI_OVERLAY_HPP
#define HEADER_GALAPIX_SDL_IMGUI_OVERLAY_HPP

#include <SDL.h>
#include <array>
#include <string>

namespace galapix {

class Viewer;

/** Dear ImGui chrome (status + tool bar) on the SDL OpenGL viewer.
 *
 *  Toggle all chrome with Tab or F1. When visible, ImGui captures input
 *  over its windows; image tools keep working on the rest of the view.
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
    Count
  };

  bool load_icons();
  void destroy_icons();
  static std::string data_root();
  bool load_icon_png(IconId id, std::string const& filename);

  bool m_initialized = false;
  bool m_visible = true;
  bool m_icons_loaded = false;
  std::array<Icon, static_cast<size_t>(IconId::Count)> m_icons{};
};

} // namespace galapix

#endif

/* EOF */
