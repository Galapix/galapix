// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "sdl/imgui_overlay.hpp"


#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>

#include "galapix/viewer.hpp"
#include "galapix/viewer_state.hpp"
#include "galapix/workspace.hpp"

namespace galapix {

ImguiOverlay::ImguiOverlay() = default;

ImguiOverlay::~ImguiOverlay()
{
  shutdown();
}

bool
ImguiOverlay::init(SDL_Window* window, SDL_GLContext gl_context)
{
  if (m_initialized || !window || !gl_context) {
    return false;
  }

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  // Chrome only — not a docking IDE shell (keep spatial canvas primary).
  io.IniFilename = nullptr;

  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 4.0f;
  style.FrameRounding = 2.0f;

  if (!ImGui_ImplSDL2_InitForOpenGL(window, gl_context)) {
    ImGui::DestroyContext();
    return false;
  }
  // Core profile-ish desktop GL; wstdisplay uses modern GL via glad.
  if (!ImGui_ImplOpenGL3_Init("#version 130")) {
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    return false;
  }

  m_initialized = true;
  return true;
}

void
ImguiOverlay::shutdown()
{
  if (!m_initialized) {
    return;
  }
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();
  m_initialized = false;
}

bool
ImguiOverlay::process_event(SDL_Event const& event)
{
  if (!m_initialized) {
    return false;
  }
  ImGui_ImplSDL2_ProcessEvent(&event);
  ImGuiIO const& io = ImGui::GetIO();
  if (event.type == SDL_MOUSEMOTION || event.type == SDL_MOUSEBUTTONDOWN ||
      event.type == SDL_MOUSEBUTTONUP || event.type == SDL_MOUSEWHEEL) {
    return m_visible && io.WantCaptureMouse;
  }
  if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP ||
      event.type == SDL_TEXTINPUT) {
    return m_visible && io.WantCaptureKeyboard;
  }
  return false;
}

void
ImguiOverlay::begin_frame()
{
  if (!m_initialized) {
    return;
  }
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();
}

void
ImguiOverlay::draw_status(Viewer& viewer)
{
  if (!m_initialized || !m_visible) {
    return;
  }

  ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Galapix status", &m_visible,
                    ImGuiWindowFlags_NoCollapse)) {
    ImGui::End();
    return;
  }

  ViewerState const& st = viewer.get_state();
  ImGui::Text("view scale: %.4f", st.get_scale());
  ImGui::Text("offset: (%.1f, %.1f)", st.get_offset().x(), st.get_offset().y());

  if (Workspace* ws = viewer.get_workspace()) {
    int requests = 0, uploads = 0, cache_entries = 0;
    ws->tile_load_stats(requests, uploads, cache_entries);
    ImGui::Separator();
    ImGui::Text("tiles");
    ImGui::Text("  pending requests: %d", requests);
    ImGui::Text("  pending uploads:  %d", uploads);
    ImGui::Text("  cache entries:    %d", cache_entries);

    int ov_idle = 0, ov_loading = 0, ov_ready = 0, ov_failed = 0;
    ws->overview_stats(ov_idle, ov_loading, ov_ready, ov_failed);
    ImGui::Separator();
    ImGui::Text("overview (stdio)");
    ImGui::Text("  idle=%d  loading=%d  ready=%d  failed=%d",
                ov_idle, ov_loading, ov_ready, ov_failed);
  }

  ImGui::Separator();
  ImGui::TextDisabled("F1 toggle  |  l console dump  |  chrome only");
  ImGui::End();
}

void
ImguiOverlay::end_frame()
{
  if (!m_initialized) {
    return;
  }
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

bool
ImguiOverlay::want_capture_mouse() const
{
  return m_initialized && m_visible && ImGui::GetIO().WantCaptureMouse;
}

bool
ImguiOverlay::want_capture_keyboard() const
{
  return m_initialized && m_visible && ImGui::GetIO().WantCaptureKeyboard;
}

} // namespace galapix

/* EOF */
