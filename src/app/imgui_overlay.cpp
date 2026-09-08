// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "app/imgui_overlay.hpp"

#include "app/galapix_paths.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <unistd.h>
#include <vector>

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

ImguiOverlay* ImguiOverlay::s_instance = nullptr;

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
    "layout_solve.png",
    "layout_spiral.png",
    "layout_vertical.png",
    "status.png",
    "help.png",
  };
  return names[index];
}

std::filesystem::path
executable_dir()
{
  char buf[4096];
  ssize_t const n = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
  if (n <= 0) {
    return {};
  }
  buf[n] = '\0';
  return std::filesystem::path(buf).parent_path();
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
  // Build-tree default from CMake (generated PNG icons live here).
#ifdef GALAPIX_DEFAULT_DATADIR
  {
    std::filesystem::path const built = GALAPIX_DEFAULT_DATADIR;
    if (std::filesystem::is_directory(built / "icons" / "hicolor")) {
      return built.string();
    }
  }
#endif
  auto has_action_icons = [](std::filesystem::path const& root) {
    return std::filesystem::is_directory(
      root / "icons" / "hicolor" / "24x24" / "actions");
  };
  if (char const* src = std::getenv("GALAPIX_SOURCE"); src && *src) {
    std::filesystem::path p = std::filesystem::path(src) / "data";
    if (has_action_icons(p)) {
      return p.string();
    }
  }
  if (has_action_icons("data")) {
    return "data";
  }
  // Installed package: $prefix/bin/galapix → $prefix/share/galapix
  std::filesystem::path const exe = executable_dir();
  if (!exe.empty()) {
    std::filesystem::path const share = exe.parent_path() / "share" / "galapix";
    if (std::filesystem::is_directory(share / "icons" / "hicolor" / "24x24" / "actions")) {
      return share.string();
    }
  }
  return "/usr/local/share/galapix";
}

bool
ImguiOverlay::load_icon_png(IconId id, std::string const& filename)
{
  std::filesystem::path const path =
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
  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, rgba->w, rgba->h, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, rgba->pixels);
  glBindTexture(GL_TEXTURE_2D, 0);

  Icon& icon = m_icons[static_cast<size_t>(id)];
  icon.id = tex;
  icon.w = rgba->w;
  icon.h = rgba->h;
  SDL_FreeSurface(rgba);
  log_info("ImGui icon loaded: {} ({}x{}, tex={})", path.string(), icon.w, icon.h, icon.id);
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
  if (!any) {
    log_warn("ImGui: no tool icons loaded (data_root={})", data_root());
  }
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
  // Do not enable keyboard nav: it sets WantCaptureKeyboard whenever an
  // ImGui window is focused (e.g. after a toolbar/help click) and would
  // swallow Viewer shortcuts until the user clicks the canvas again.
  io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.WindowRounding = 0.0f;
  style.FrameRounding = 4.0f;
  style.WindowBorderSize = 0.0f;
  style.WindowPadding = ImVec2(6.0f, 8.0f);
  style.ItemSpacing = ImVec2(4.0f, 6.0f);

  if (!ImGui_ImplSDL2_InitForOpenGL(window, gl_context)) {
    ImGui::DestroyContext();
    return false;
  }
  if (!ImGui_ImplOpenGL3_Init("#version 130")) {
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    return false;
  }

  // GL context is current; upload icon textures now.
  load_icons();
  m_initialized = true;
  s_instance = this;
  return true;
}

void
ImguiOverlay::shutdown()
{
  if (!m_initialized) {
    return;
  }
  if (s_instance == this) {
    s_instance = nullptr;
  }
  m_toasts.clear();
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
  // Only swallow keyboard when ImGui actually needs text entry (none yet).
  // WantCaptureKeyboard alone is true after focusing toolbar/help windows and
  // would block Viewer shortcuts (p/z/1–6/…) until the canvas is re-clicked.
  if (event.type == SDL_TEXTINPUT) {
    return m_visible && io.WantTextInput;
  }
  if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
    return m_visible && io.WantTextInput;
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
ImguiOverlay::panel_toggle_btn(IconId id, char const* id_str, char const* tip_on,
                               char const* tip_off, bool& visible_flag)
{
  Icon const& icon = m_icons[static_cast<size_t>(id)];
  ImGui::PushID(id_str);
  bool pressed = false;
  ImVec2 const sz(32.0f, 32.0f);
  if (icon.id) {
    ImTextureID const tex_id =
      static_cast<ImTextureID>(static_cast<std::uintptr_t>(icon.id));
    if (visible_flag) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.45f, 0.70f, 1.0f));
    }
    pressed = ImGui::ImageButton(id_str, tex_id, sz);
    if (visible_flag) {
      ImGui::PopStyleColor();
    }
  } else {
    char label[2] = { id_str[0], '\0' };
    if (visible_flag) {
      label[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(label[0])));
    }
    pressed = ImGui::Button(label, sz);
  }
  if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
    ImGui::SetTooltip("%s", visible_flag ? tip_on : tip_off);
  }
  if (pressed) {
    visible_flag = !visible_flag;
  }
  ImGui::PopID();
}

void
ImguiOverlay::notify(std::string message, float duration_seconds)
{
  if (!s_instance || message.empty()) {
    return;
  }
  auto const now = std::chrono::steady_clock::now();
  auto const dur = std::chrono::duration_cast<std::chrono::steady_clock::duration>(
    std::chrono::duration<float>(duration_seconds));
  s_instance->m_toasts.push_back(Toast{std::move(message), now + dur});
  while (s_instance->m_toasts.size() > kMaxToasts) {
    s_instance->m_toasts.pop_front();
  }
  // Keep the frame loop alive so the toast can fade out without user input.
  if (Viewer* v = Viewer::current()) {
    v->redraw();
  }
}

void
ImguiOverlay::draw_toasts()
{
  if (!m_initialized) {
    return;
  }

  auto const now = std::chrono::steady_clock::now();
  while (!m_toasts.empty() && m_toasts.front().expires <= now) {
    m_toasts.pop_front();
  }
  if (m_toasts.empty()) {
    return;
  }

  ImGuiIO const& io = ImGui::GetIO();
  float const pad = 12.0f;
  float y = io.DisplaySize.y - pad;

  // Still animating — request another frame for fade/expiry.
  if (Viewer* v = Viewer::current()) {
    v->redraw();
  }

  int idx = 0;
  for (auto it = m_toasts.rbegin(); it != m_toasts.rend(); ++it, ++idx) {
    float const remaining = std::chrono::duration<float>(it->expires - now).count();
    float alpha = 1.0f;
    if (remaining < 0.5f) {
      alpha = std::max(0.0f, remaining / 0.5f);
    }

    ImVec2 const text_size = ImGui::CalcTextSize(it->text.c_str());
    float const win_w = text_size.x + 24.0f;
    float const win_h = text_size.y + 16.0f;
    y -= win_h + 6.0f;
    float const x = (io.DisplaySize.x - win_w) * 0.5f;

    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(win_w, win_h), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.82f * alpha);
    ImGuiWindowFlags const flags =
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
      ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoInputs;

    char id[32];
    std::snprintf(id, sizeof(id), "##toast%d", idx);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12.0f, 8.0f));
    if (ImGui::Begin(id, nullptr, flags)) {
      ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, alpha));
      ImGui::TextUnformatted(it->text.c_str());
      ImGui::PopStyleColor();
    }
    ImGui::End();
    ImGui::PopStyleVar(2);
  }
}

void
ImguiOverlay::draw_status(Viewer& viewer)
{
  if (!m_initialized) {
    return;
  }

  // Toasts stay visible even when Tab/F1 chrome is hidden.
  draw_toasts();

  if (!m_visible) {
    return;
  }

  ImGuiIO const& io = ImGui::GetIO();
  float const bar_w = 48.0f;

  // --- Left vertical toolbar ---
  ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ImVec2(bar_w, io.DisplaySize.y), ImGuiCond_Always);
  ImGuiWindowFlags const tb_flags =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
    ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
    ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.08f, 0.10f, 0.92f));
  if (ImGui::Begin("##toolbar", nullptr, tb_flags)) {
    auto tool_btn = [&](IconId id, char const* tip, auto&& action) {
      Icon const& icon = m_icons[static_cast<size_t>(id)];
      ImGui::PushID(static_cast<int>(id));
      bool pressed = false;
      ImVec2 const sz(32.0f, 32.0f);
      if (icon.id) {
        ImTextureID const tex_id =
          static_cast<ImTextureID>(static_cast<std::uintptr_t>(icon.id));
        pressed = ImGui::ImageButton(tip, tex_id, sz);
      } else {
        // Fallback: first letter of tip
        char label[2] = { tip[0], '\0' };
        pressed = ImGui::Button(label, sz);
      }
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip("%s", tip);
      }
      if (pressed) {
        action();
      }
      ImGui::PopID();
    };

    tool_btn(IconId::Pan, "Pan (p)", [&] { viewer.set_pan_tool(); });
    tool_btn(IconId::ZoomRect, "Zoom rect (z)", [&] { viewer.set_zoom_tool(); });
    tool_btn(IconId::GridTool, "Grid tool (y)", [&] { viewer.set_grid_tool(); });
    tool_btn(IconId::Move, "Move/resize (m)", [&] { viewer.set_move_resize_tool(); });

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    tool_btn(IconId::Grid, "Toggle grid (g)", [&] { viewer.toggle_grid(); });
    tool_btn(IconId::GridPin, "Pin grid (f)", [&] { viewer.toggle_pinned_grid(); });

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    tool_btn(IconId::LayoutRegular, "Layout regular (1)", [&] { viewer.layout_auto(); });
    tool_btn(IconId::LayoutTight, "Layout tight (2)", [&] { viewer.layout_tight(); });
    tool_btn(IconId::LayoutRandom, "Layout random (3)", [&] { viewer.layout_random(); });
    tool_btn(IconId::LayoutSolve, "Solve overlaps (4)", [&] { viewer.layout_solve_overlaps(); });
    tool_btn(IconId::LayoutSpiral, "Layout spiral (5)", [&] { viewer.layout_spiral(); });
    tool_btn(IconId::LayoutVertical, "Layout vertical (6)", [&] { viewer.layout_vertical(); });

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    panel_toggle_btn(IconId::Status, "status_toggle",
                     "Hide status", "Show status", m_status_visible);
    panel_toggle_btn(IconId::Help, "help_toggle",
                     "Hide help", "Show help / shortcuts", m_help_visible);
  }
  ImGui::End();
  ImGui::PopStyleColor();

  if (m_status_visible) {
    draw_status_panel(viewer);
  }
  if (m_help_visible) {
    draw_help_panel(viewer);
  }
}

void
ImguiOverlay::draw_status_panel(Viewer& viewer)
{
  float const bar_w = 48.0f;
  ImGui::SetNextWindowPos(ImVec2(bar_w + 12.0f, 12.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(320.0f, 280.0f), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Status", &m_status_visible, ImGuiWindowFlags_NoCollapse)) {
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

  ImGui::End();
}

void
ImguiOverlay::draw_help_panel(Viewer& viewer)
{
  float const bar_w = 48.0f;
  // Place help a bit lower / right of status so both can stay open.
  ImGui::SetNextWindowPos(ImVec2(bar_w + 12.0f, 300.0f), ImGuiCond_FirstUseEver);
  ImGui::SetNextWindowSize(ImVec2(420.0f, 480.0f), ImGuiCond_FirstUseEver);
  if (!ImGui::Begin("Help / Shortcuts", &m_help_visible, ImGuiWindowFlags_NoCollapse)) {
    ImGui::End();
    return;
  }

  ImGui::TextWrapped(
    "Click a key row to run the action (same as the keyboard shortcut). "
    "Chrome-only and mouse bindings are listed but not clickable.");

  if (ImGui::BeginTable("shortcuts", 2,
                        ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg |
                        ImGuiTableFlags_SizingStretchProp,
                        ImVec2(-1.0f, 0.0f))) {
    ImGui::TableSetupColumn("Key", ImGuiTableColumnFlags_WidthFixed, 110.0f);
    ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthStretch);

    auto section = [](char const* title) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextDisabled("%s", title);
      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted("");
    };

    // Non-actionable reference row (chrome / mouse / app-shell keys).
    auto info_row = [](char const* key, char const* desc) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted(key);
      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted(desc);
    };

    // Clickable row: Selectable on the key triggers the Viewer action.
    auto action_row = [](char const* key, char const* desc, auto&& action) {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::PushID(key);
      bool const clicked = ImGui::Selectable(key, false);
      if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) {
        ImGui::SetTooltip("Click to run: %s", desc);
      }
      ImGui::TableSetColumnIndex(1);
      ImGui::TextUnformatted(desc);
      if (clicked) {
        action();
      }
      ImGui::PopID();
    };

    section("Chrome");
    info_row("Tab / F1", "Show/hide toolbar (+ panels)");
    info_row("Status btn", "Show/hide Status panel");
    info_row("Help btn", "Show/hide this Help panel");

    section("Tools");
    action_row("p", "Pan tool", [&] { viewer.set_pan_tool(); });
    action_row("z", "Zoom rect tool", [&] { viewer.set_zoom_tool(); });
    action_row("y", "Grid tool", [&] { viewer.set_grid_tool(); });
    action_row("m", "Move/resize tool", [&] { viewer.set_move_resize_tool(); });
    info_row("LMB drag", "Use active tool");
    info_row("MMB / Space+LMB", "Pan while held");
    info_row("RMB + mouse", "Rotate view while held");
    info_row("Wheel", "Zoom toward cursor");

    section("View");
    action_row("h", "Zoom home (fit)", [&] { viewer.zoom_home(); });
    action_row("d", "Zoom to selection", [&] { viewer.zoom_to_selection(); });
    action_row("g", "Toggle grid", [&] { viewer.toggle_grid(); });
    action_row("f", "Toggle pinned grid", [&] { viewer.toggle_pinned_grid(); });
    action_row("b", "Cycle background color", [&] { viewer.toggle_background_color(); });
    action_row("Shift+b", "Cycle background (reverse)",
               [&] { viewer.toggle_background_color(true); });
    action_row("Left", "Rotate view −90°", [&] { viewer.rotate_view_270(); });
    action_row("Right", "Rotate view +90°", [&] { viewer.rotate_view_90(); });
    action_row("Up / Down", "Reset view rotation", [&] { viewer.reset_view_rotation(); });
    info_row("Numpad 8/2/4/6", "Nudge view");
    info_row("Numpad +/-", "Zoom in/out (center)");
    info_row("v", "Tile debug overlay (green/cyan/purple)");
    info_row("u", "Toggle tile requests (cache-only mode)");
    info_row("F11", "Toggle fullscreen");
    action_row("t", "Toggle trackball mode", [&] { viewer.toggle_trackball_mode(); });

    section("Layout");
    action_row("1", "Regular layout", [&] { viewer.layout_auto(); });
    action_row("2", "Tight layout", [&] { viewer.layout_tight(); });
    action_row("3", "Random layout", [&] { viewer.layout_random(); });
    action_row("4", "Solve overlaps", [&] { viewer.layout_solve_overlaps(); });
    action_row("5", "Spiral layout", [&] { viewer.layout_spiral(); });
    action_row("6", "Vertical layout", [&] { viewer.layout_vertical(); });
    action_row("s", "Sort by name", [&] { viewer.sort_image_list(); });
    action_row("Shift+s", "Sort reverse", [&] { viewer.sort_reverse_image_list(); });
    action_row("n", "Shuffle", [&] { viewer.shuffle_image_list(); });

    section("Selection");
    action_row("i", "Isolate selection", [&] { viewer.isolate_selection(); });
    action_row("Delete", "Remove selection from workspace",
               [&] { viewer.delete_selection(); });
    action_row("F5", "Refresh selection", [&] { viewer.refresh_selection(); });

    section("Image / cache");
    action_row("F2", "Load workspace", [&] { viewer.load(); });
    action_row("F3", "Save workspace", [&] { viewer.save(); });
    action_row("c", "Clear tile cache", [&] { viewer.clear_cache(); });
    action_row("k", "Cleanup cache", [&] { viewer.cleanup_cache(); });
    action_row("F6", "Brightness +", [&] { viewer.increase_brightness(); });
    action_row("F7", "Brightness −", [&] { viewer.decrease_brightness(); });
    action_row("F8", "Contrast +", [&] { viewer.increase_contrast(); });
    action_row("F9", "Contrast −", [&] { viewer.decrease_contrast(); });
    action_row("PgUp", "Gamma +", [&] { viewer.increase_gamma(); });
    action_row("PgDn", "Gamma −", [&] { viewer.decrease_gamma(); });
    action_row("F10", "Reset gamma", [&] { viewer.reset_gamma(); });
    info_row("F12", "Screenshot → /tmp/");

    section("Debug");
    action_row("Space", "Print visible images", [&] { viewer.print_images(); });
    action_row("l", "Print viewer state", [&] { viewer.print_state(); });
    action_row("0", "Print info", [&] { viewer.print_info(); });
    info_row("Esc", "Quit");

    ImGui::EndTable();
  }

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
  return m_initialized && m_visible && ImGui::GetIO().WantTextInput;
}

} // namespace galapix

/* EOF */
