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

namespace galapix {

class Viewer;

/** Dear ImGui chrome overlay (placeholder UI on pure OpenGL).
 *
 *  Toggle with F1. When visible, ImGui captures mouse/keyboard over its
 *  windows so the image tools keep working on the rest of the view.
 *  Application state stays in Viewer/Workspace; ImGui only displays and
 *  sends simple commands.
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
  bool m_initialized = false;
  bool m_visible = true;
};

} // namespace galapix

#endif

/* EOF */
