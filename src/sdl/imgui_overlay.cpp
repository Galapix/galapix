// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "sdl/imgui_overlay.hpp"

#include <cstdlib>
#include <filesystem>
#include <string>

#include <SDL_image.h>
#include <glad/gl.h>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl2.h>
#include <logmich/log.hpp>

#include "galapix/viewer.hpp"
#include "galapix/viewer_state.hpp"
#include "galapix/workspace.hpp"

namespace galapix {
namespace {

char const* icon_file(int index)
{
  static char const* names[] = {
    "pan_tool.png",
    "zoom_rect_tool.png",
    "grid_tool.png",
    "move_tool.png",
    "grid.png",
    "grid_pin.png",
    "layout_regular.png",
    "layout_tight.png",
    "layout_random.png",
  };
  return names[index];
}

} // namespace

ImguiOverlay::ImguiOverlay() = default;

ImguiOverlay::~ImguiOverlay()
{
  shutdown();
}

std::string
ImguiOverlay::data_root()
{
  if (char const* env = std::getenv("GALAPIX_DATADIR"); env && *env) {
    return env;
  }
  if (std::filesystem::is_directory("data/icons")) {
    return "data";
  }
  return "/usr/local/share/galapix";
}

bool
ImguiOverlay::load_icon_png(IconId id, std::string const& filename)
{
  std::filesystem::path path =
    std::filesystem::path(data_root()) / "icons" / "hicolor" / "24x24" / "actions" / filename;

  SDL_Surface* surface = IMG_Load(path.string().c_str());
  if (!surface) {
    log_warn("ImGui icon load failed: {} ({})", path.string(), IMG_GetError());
    return false;
  }

  SDL_Surface* rgba = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);
  SDL_FreeSurface(surface);
  if (!rgba) {
    log_warn("ImGui icon convert failed: {}", path.string());
    return false;
  }

  GLuint tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, rgba->pixels);

  Icon& icon = m_icons[static_cast<size_t>(id)];
  icon.id = tex;
  icon.w = rgba->w;
  icon.h = rgba->h;
  SDL_FreeSurface(rgba);
  return true;
}

bool
ImguiOverlay::load_icons()
{
  destroy_icons();
  bool any = false;
  for (int i = 0; i < static_cast<int>(IconId::Count); ++i) {
    auto id = static_cast<IconId>(i);
    if (load_icon_png(id, icon_file(i))) {
      any = true;
    }
  }
  m_icons_loaded = any;
  return any;
}

void
ImguiOverlay::destroy_icons()
{
  for (Icon& icon : m_icons) {
    if (icon.id) {
      GLuint tex = icon.id;
      glDeleteTextures(1, &tex);
      icon = {};
    }
  }
  m_icons_loaded = false;
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

  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 6.0f;
  style.FrameRounding = 4.0f;

  if (!ImGui_ImplSDL2_InitForOpenGL(window, gl_context)) {
    ImGui::DestroyContext();
    return false;
  }
  if (!ImGui_ImplOpenGL3_Init("#version 130")) {
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    return false;
  }

  load_icons();
  m_initialized = true;
  return true;
}

void
ImguiOverlay::shutdown()
{
  if (!m_initialized) {
    return;
  }
  destroy_icons();
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

  // --- Tools (replaces old GTK toolbar) ---
  ImGui::SetNextWindowPos(ImVec2(12.0f, 12.0f), ImGuiCond_FirstUseEver);
  if (ImGui::Begin("Tools", nullptr,
                   ImGuiWindowFlags_AlwaysAutoResize |
                   ImGuiWindowFlags_NoCollapse)) {
    auto tool_btn = [&](IconId id, char const* tip, auto&& action) {
      Icon const& icon = m_icons[static_cast<size_t>(id)];
      ImGui::PushID(static_cast<int>(id));
      bool pressed = false;
      if (icon.id) {
        pressed = ImGui::ImageButton(
          tip,
          reinterpret_cast<ImTextureID>(static_cast<intptr_t>(icon.id)),
          ImVec2(static_cast<float>(icon.w), static_cast<float>(icon.h)));
      } else {
        pressed = ImGui::Button(tip, ImVec2(28.0f, 28.0f));
      }
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("%s", tip);
      }
      if (pressed) {
        action();
      }
      ImGui::PopID();
    };

    tool_btn(IconId::Pan, "Pan (p)", [&] { viewer.set_pan_tool(); });
    ImGui::SameLine();
    tool_btn(IconId::ZoomRect, "Zoom rect (z)", [&] { viewer.set_zoom_tool(); });
    ImGui::SameLine();
    tool_btn(IconId::GridTool, "Grid tool (y)", [&] { viewer.set_grid_tool(); });
    ImGui::SameLine();
    tool_btn(IconId::Move, "Move/resize (m)", [&] { viewer.set_move_resize_tool(); });

    ImGui::Separator();

    tool_btn(IconId::Grid, "Toggle grid (g)", [&] { viewer.toggle_grid(); });
    ImGui::SameLine();
    tool_btn(IconId::GridPin, "Pin grid (f)", [&] { viewer.toggle_pinned_grid(); });

    ImGui::Separator();

    tool_btn(IconId::LayoutRegular, "Layout regular (1)", [&] { viewer.layout_auto(); });
    ImGui::SameLine();
    tool_btn(IconId::LayoutTight, "Layout tight (2)", [&] { viewer.layout_tight(); });
    ImGui::SameLine();
    tool_btn(IconId::LayoutRandom, "Layout random (3)", [&] { viewer.layout_random(); });
  }
  ImGui::End();

  // --- Status ---
  ImGui::SetNextWindowPos(ImVec2(12.0f, 120.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(360.0f, 0.0f), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Galapix status", &m_visible, ImGuiWindowFlags_NoCollapse)) {
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
  ImGui::TextDisabled("Tab / F1 hide chrome  |  H zoom home  |  l console dump");
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
