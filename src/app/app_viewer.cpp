// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "app/app_viewer.hpp"
#include "app/imgui_overlay.hpp"
#include "app/key_bindings.hpp"

#include <string>

#include <format>
#include <iostream>
#include <thread>
#include <cmath>
#include <logmich/log.hpp>
#include <SDL_keycode.h>

#include <surf/color.hpp>
#include <surf/transform.hpp>
#include <surf/plugins/png.hpp>
#include <wstsystem/system.hpp>
#include <wstdisplay/opengl_window.hpp>
#include <wstdisplay/renderer.hpp>

#include "galapix/viewer.hpp"
#include "galapix/image_tiles.hpp"
#include "galapix/viewer_state.hpp"
#include "galapix/workspace.hpp"
#include "spnav/space_navigator.hpp"
#include "util/filesystem.hpp"

namespace galapix {

#ifdef HAVE_SPACE_NAVIGATOR
#  include <spnav.h>
#endif

using namespace surf;

namespace {

Key sdlkey2viewer(SDL_Keycode key)
{
  switch(key)
  {
    case SDLK_END:
      return Key::ZOOM_OUT;

    case SDLK_HOME:
      return Key::ZOOM_IN;

    case SDLK_RSHIFT:
    case SDLK_LSHIFT:
      return Key::ROTATE;

    default:
      return Key::NO_KEY;
  }
}

float deadzone(float value, float threshold)
{
  if (fabsf(value) < threshold)
  {
    return 0.0f;
  }
  else if (value < 0)
  {
    return (value + threshold) / (1.0f - threshold);
  }
  else
  {
    return (value - threshold) / (1.0f - threshold);
  }
}

} // namespace

AppViewer::AppViewer(Size const& size, bool fullscreen, int  anti_aliasing,
                     Viewer& viewer) :
  m_system(std::make_unique<wstsystem::System>()),
  m_window(m_system->create_window({
    .title = "Galapix",
    .icon = {},
    .size = size,
    .mode = fullscreen ? wstdisplay::OpenGLWindow::Mode::FullscreenDesktop : wstdisplay::OpenGLWindow::Mode::Window,
    .resizable = true,
    .anti_aliasing = anti_aliasing,
    // Pace the loop with display refresh. A fixed SDL_Delay(10) previously
    // left only ~6ms of budget for draw+upload at 60Hz and guaranteed jank.
    .vsync = true})),
  m_canvas(),
  m_viewer(viewer),
  m_quit(false),
  m_fullscreen(fullscreen),
  m_spnav_allow_rotate(false),
  m_imgui(),
  m_key_bindings(),
  m_gamecontrollers()
{
  m_key_bindings = make_key_bindings();
  m_imgui.set_key_bindings(&m_key_bindings);

  // The Renderer takes the frame size in begin_frame(), only the Viewer
  // needs to know about window resizes.
  m_window->sig_resized.connect([this](geom::isize const& new_size) {
    m_viewer.reshape(new_size);
  });
  m_viewer.reshape(m_window->get_size());

  SDL_Window* sdl_window = SDL_GetWindowFromID(m_window->get_id());
  SDL_GLContext glctx = SDL_GL_GetCurrentContext();
  if (!m_imgui.init(sdl_window, glctx)) {
    log_warn("ImGui overlay init failed; continuing without HUD");
  }
}

AppViewer::~AppViewer()
{
  // Tile and overview textures belong to the window's Device, release
  // them while it is still around.
  m_viewer.clear_cache();

  m_imgui.shutdown();
  for(SDL_GameController* gamecontroller: m_gamecontrollers)
  {
    SDL_GameControllerClose(gamecontroller);
  }
}

void
AppViewer::process_event(SDL_Event const& event)
{
  // Tab / F1 always toggle chrome, even when ImGui wants keyboard focus.
  // (H remains zoom-home; do not steal it for UI hide.)
  if (event.type == SDL_KEYDOWN) {
    SDL_Keycode const sym = event.key.keysym.sym;
    if (sym == SDLK_F1 || sym == SDLK_TAB) {
      m_imgui.toggle();
      return;
    }
  }

  // Feed ImGui; skip viewer handling when it wants capture.
  if (m_imgui.process_event(event)) {
    return;
  }

  Uint8 const* keystate = SDL_GetKeyboardState(nullptr);

  switch(event.type)
  {
    case SDL_USEREVENT:
      if (event.user.code == 0)
      {
#ifndef HAVE_SPACE_NAVIGATOR
        assert(false && "Broken build, this code should be unreachable");
#else
        std::unique_ptr<spnav_event> spnav_ev(static_cast<spnav_event*>(event.user.data1));

        switch(spnav_ev->type)
        {
          case SPNAV_EVENT_MOTION:
            {
              if (false)
              {
                log_debug("MotionEvent: ({}, {}, {}) ({}, {}, {})",
                          spnav_ev->motion.x,
                          spnav_ev->motion.y,
                          spnav_ev->motion.z,

                          spnav_ev->motion.rx,
                          spnav_ev->motion.ry,
                          spnav_ev->motion.rz);
              }

              float factor = static_cast<float>(-abs(spnav_ev->motion.y))/10000.0f;

              if (spnav_ev->motion.y > 0)
                m_viewer.get_state().zoom(1.0f+factor);
              else if (spnav_ev->motion.y < 0)
                m_viewer.get_state().zoom(1.0f/(1.0f+factor));

              m_viewer.get_state().move(Vector2f(static_cast<float>(-spnav_ev->motion.x) / 10.0f,
                                                 static_cast<float>(+spnav_ev->motion.z) / 10.0f));

              if (m_spnav_allow_rotate)
                m_viewer.get_state().rotate(static_cast<float>(spnav_ev->motion.ry) / 200.0f);
            }
            break;

          case SPNAV_EVENT_BUTTON:
            if (false)
            {
              std::cout << "ButtonEvent: " << spnav_ev->button.press << spnav_ev->button.bnum << std::endl;
            }

            if (spnav_ev->button.bnum == 0 && spnav_ev->button.press)
              m_viewer.get_state().set_angle(0.0f);

            if (spnav_ev->button.bnum == 1 && spnav_ev->button.press)
              m_spnav_allow_rotate = !m_spnav_allow_rotate;
            break;

          default:
            assert(false && "SpaceNavigator: Unhandled event");
        }
#endif
      }
      else if (event.user.code == 1)
      {
        // New tile arrived
      }
      break;

    case SDL_JOYAXISMOTION:
      break;

    case SDL_JOYBALLMOTION:
      break;

    case SDL_JOYHATMOTION:
      break;

    case SDL_JOYBUTTONDOWN:
      break;

    case SDL_JOYBUTTONUP:
      break;

    case SDL_CONTROLLERAXISMOTION:
      break;

    case SDL_CONTROLLERBUTTONDOWN:
      break;

    case SDL_CONTROLLERBUTTONUP:
      break;

    case SDL_CONTROLLERDEVICEADDED:
      printf("SDL_CONTROLLERDEVICEADDED which:%d\n", event.cdevice.which);
      add_gamecontroller(event.cdevice.which);
      break;

    case SDL_CONTROLLERDEVICEREMOVED:
      printf("SDL_CONTROLLERDEVICEREMOVED which:%d\n",  event.cdevice.which);
      remove_gamecontroller(event.cdevice.which);
      break;

    case SDL_CONTROLLERDEVICEREMAPPED:
      printf("SDL_CONTROLLERDEVICEREMAPPED which:%d\n", event.cdevice.which);
      break;

    case SDL_DROPFILE:
      {
        char* dropped = event.drop.file;
        if (dropped) {
          std::vector<std::string> paths{std::string(dropped)};
          SDL_free(dropped);
          m_viewer.open_paths(paths);
        }
      }
      break;

    case SDL_QUIT:

      std::cout << "Viewer: SDL_QUIT received" << std::endl;
      m_quit = true;
      break;

      //case SDL_VIDEOEXPOSE:
      //break;

    case SDL_WINDOWEVENT:
      // Forwards RESIZED → sig_resized (Viewer::reshape).
      m_window->handle_event(event.window);
      break;

    case SDL_MOUSEMOTION:
      m_viewer.on_mouse_motion(Vector2i(event.motion.x,    event.motion.y),
                               Vector2i(event.motion.xrel, event.motion.yrel));
      break;

      // FIXME: When the mouse is set to left-hand mode, SDL reverses
      // the mouse buttons when a grab is active!
    case SDL_MOUSEBUTTONDOWN:
      m_viewer.on_mouse_button_down(Vector2i(event.button.x, event.button.y),
                                    static_cast<MouseButton>(event.button.button));
      break;

    case SDL_MOUSEBUTTONUP:
      switch(event.button.button)
      {
        default:
          m_viewer.on_mouse_button_up(Vector2i(event.button.x, event.button.y),
                                      static_cast<MouseButton>(event.button.button));
          break;
      }
      break;

    case SDL_MOUSEWHEEL:
      {
        // SDL_MOUSEWHEEL does not carry coordinates in event.button (union
        // garbage / zeros → zoom pinned to top-left). Use current pointer.
        int mx = 0;
        int my = 0;
        SDL_GetMouseState(&mx, &my);
#if SDL_VERSION_ATLEAST(2, 26, 0)
        if (event.wheel.mouseX != 0 || event.wheel.mouseY != 0) {
          mx = event.wheel.mouseX;
          my = event.wheel.mouseY;
        }
#endif
        Vector2i const pos(mx, my);
        for (int i = event.wheel.y; i < 0; ++i) {
          m_viewer.get_state().zoom(1.1f, pos);
        }
        for (int i = event.wheel.y; i > 0; --i) {
          m_viewer.get_state().zoom(1.0f / 1.1f, pos);
        }
      }
      break;

    case SDL_KEYDOWN:
      {
        bool const shift = keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT];
        if (KeyBinding const* binding = find_key_binding(m_key_bindings, event.key.keysym.sym, shift)) {
          binding->action();
        } else {
          // held keys: Home/End zoom, Shift rotates the view
          m_viewer.on_key_down(sdlkey2viewer(event.key.keysym.sym));
        }
      }
      break;

    case SDL_KEYUP:
      m_viewer.on_key_up(sdlkey2viewer(event.key.keysym.sym));
      break;

    default:
      break;
  }
}

void
AppViewer::draw_frame()
{
  wstdisplay::Renderer& renderer = m_window->get_renderer();
  renderer.begin_frame(m_window->get_drawable_size());
  m_viewer.draw(renderer, m_canvas);
  // Restores the GL state for ImGui
  renderer.end_frame();

  m_imgui.begin_frame();
  m_imgui.draw_status(m_viewer);
  m_imgui.end_frame();
}

KeyBindings
AppViewer::make_key_bindings()
{
  Viewer& v = m_viewer;
  auto nudge = [&v](float x, float y) {
    v.get_state().set_offset(v.get_state().get_offset().as_vec() + Vector2f(x, y).as_vec());
  };

  return {
    {SDLK_p, false, "p", "Tools", "Pan & zoom: LMB zoom in, MMB pan, RMB zoom out", [&v] { v.set_pan_tool(); }},
    {SDLK_z, false, "z", "Tools", "Zoom rectangle on LMB", [&v] { v.set_zoom_tool(); }},
    {SDLK_y, false, "y", "Tools", "Grid rectangle on LMB", [&v] { v.set_grid_tool(); }},
    {SDLK_m, false, "m", "Tools", "Move/select on LMB, resize selection on RMB", [&v] { v.set_move_resize_tool(); }},
    {SDLK_r, false, "r", "Tools", "Move/select on LMB, rotate selection on RMB", [&v] { v.set_move_rotate_tool(); }},
    {SDLK_t, false, "t", "Tools", "Toggle trackball mode", [&v] { v.toggle_trackball_mode(); }},

    {SDLK_h, false, "h", "View", "Zoom home", [&v] { v.zoom_home(); }},
    {SDLK_d, false, "d", "View", "Zoom to selection (or everything)", [&v] { v.zoom_to_selection(); }},
    {SDLK_KP_PLUS, false, "Numpad +", "View", "Zoom in", [&v] { v.get_state().zoom(1.25f); }},
    {SDLK_KP_MINUS, false, "Numpad -", "View", "Zoom out", [&v] { v.get_state().zoom(1.0f / 1.25f); }},
    {SDLK_KP_8, false, "Numpad 8", "View", "Nudge view up", [nudge] { nudge(0.0f, +128.0f); }},
    {SDLK_KP_2, false, "Numpad 2", "View", "Nudge view down", [nudge] { nudge(0.0f, -128.0f); }},
    {SDLK_KP_4, false, "Numpad 4", "View", "Nudge view left", [nudge] { nudge(+128.0f, 0.0f); }},
    {SDLK_KP_6, false, "Numpad 6", "View", "Nudge view right", [nudge] { nudge(-128.0f, 0.0f); }},
    {SDLK_LEFT, false, "Left", "View", "Rotate view -90°", [&v] { v.rotate_view_270(); }},
    {SDLK_RIGHT, false, "Right", "View", "Rotate view +90°", [&v] { v.rotate_view_90(); }},
    {SDLK_UP, false, "Up", "View", "Reset view rotation", [&v] { v.reset_view_rotation(); }},
    {SDLK_DOWN, false, "Down", "View", "Reset view rotation", [&v] { v.reset_view_rotation(); }},
    {SDLK_g, false, "g", "View", "Toggle grid", [&v] { v.toggle_grid(); }},
    {SDLK_f, false, "f", "View", "Toggle pinned grid", [&v] { v.toggle_pinned_grid(); }},
    {SDLK_b, false, "b", "View", "Cycle background color", [&v] { v.toggle_background_color(); }},
    {SDLK_b, true, "Shift+b", "View", "Cycle background color backwards", [&v] { v.toggle_background_color(true); }},
    {SDLK_F11, false, "F11", "View", "Toggle fullscreen", [this] { toggle_fullscreen(); }},

    {SDLK_1, false, "1", "Layout", "Regular layout", [&v] { v.layout_auto(); }},
    {SDLK_2, false, "2", "Layout", "Tight layout", [&v] { v.layout_tight(); }},
    {SDLK_3, false, "3", "Layout", "Random layout", [&v] { v.layout_random(); }},
    {SDLK_4, false, "4", "Layout", "Solve overlaps", [&v] { v.layout_solve_overlaps(); }},
    {SDLK_5, false, "5", "Layout", "Spiral layout", [&v] { v.layout_spiral(); }},
    {SDLK_6, false, "6", "Layout", "Vertical layout", [&v] { v.layout_vertical(); }},
    {SDLK_s, false, "s", "Layout", "Sort by name", [&v] { v.sort_image_list(); }},
    {SDLK_s, true, "Shift+s", "Layout", "Sort by name, reverse", [&v] { v.sort_reverse_image_list(); }},
    {SDLK_n, false, "n", "Layout", "Shuffle", [&v] { v.shuffle_image_list(); }},

    {SDLK_i, false, "i", "Selection", "Isolate selection", [&v] { v.isolate_selection(); }},
    {SDLK_DELETE, false, "Delete", "Selection", "Remove selection from workspace", [&v] { v.delete_selection(); }},
    {SDLK_F5, false, "F5", "Selection", "Refresh selection (not implemented)", [&v] { v.refresh_selection(); }},

    {SDLK_F2, false, "F2", "Workspace", "Load /tmp/workspace-dump.galapix", [&v] { v.load(); }},
    {SDLK_F3, false, "F3", "Workspace", "Save /tmp/workspace-dump.galapix", [&v] { v.save(); }},
    {SDLK_F12, false, "F12", "Workspace", "Screenshot to /tmp/", [this] { save_screenshot(); }},

    {SDLK_F6, false, "F6", "Display", "Brightness +", [&v] { v.increase_brightness(); }},
    {SDLK_F7, false, "F7", "Display", "Brightness -", [&v] { v.decrease_brightness(); }},
    {SDLK_F8, false, "F8", "Display", "Contrast +", [&v] { v.increase_contrast(); }},
    {SDLK_F9, false, "F9", "Display", "Contrast -", [&v] { v.decrease_contrast(); }},
    {SDLK_PAGEUP, false, "PgUp", "Display", "Gamma +", [&v] { v.increase_gamma(); }},
    {SDLK_PAGEDOWN, false, "PgDn", "Display", "Gamma -", [&v] { v.decrease_gamma(); }},
    {SDLK_F10, false, "F10", "Display", "Reset gamma", [&v] { v.reset_gamma(); }},

    {SDLK_c, false, "c", "Tiles", "Clear tile cache", [&v] { v.clear_cache(); }},
    {SDLK_k, false, "k", "Tiles", "Free textures of off-screen images", [&v] { v.cleanup_cache(); }},
    {SDLK_v, false, "v", "Tiles", "Tile debug overlay (tint = pyramid scale)", [this] { toggle_tile_debug(); }},
    {SDLK_u, false, "u", "Tiles", "Toggle tile requests (cache-only mode)", [this] { toggle_tile_requests(); }},

    {SDLK_SPACE, false, "Space", "Debug", "Print visible images", [&v] { v.print_images(); }},
    {SDLK_l, false, "l", "Debug", "Print viewer and tile state", [&v] { v.print_state(); }},
    {SDLK_0, false, "0", "Debug", "Print image info", [&v] { v.print_info(); }},

    {SDLK_ESCAPE, false, "Esc", "App", "Quit", [this] { m_quit = true; }},
  };
}

void
AppViewer::save_screenshot()
{
  SoftwareSurface surface = m_window->screenshot();
  // FIXME: Could do this in a worker thread to avoid pause on screenshotting
  for(int i = 0; ; ++i)
  {
    std::string outfile = std::format("/tmp/galapix-screenshot-{:04d}.png", i);
    if (!Filesystem::exist(outfile))
    {
      png::save(surface, outfile);
      std::cout << "Screenshot written to " << outfile << std::endl;
      break;
    }
  }
}

void
AppViewer::toggle_fullscreen()
{
  m_fullscreen = !m_fullscreen;
  m_window->set_mode(m_fullscreen
                     ? wstdisplay::OpenGLWindow::Mode::FullscreenDesktop
                     : wstdisplay::OpenGLWindow::Mode::Window);
}

void
AppViewer::toggle_tile_debug()
{
  bool const on = !ImageTiles::tile_debug();
  ImageTiles::set_tile_debug(on);
  std::string msg = std::string("Tile debug: ") + (on ? "ON" : "OFF")
    + " (tint=pyramid scale, green bias=exact, purple=no tile)";
  std::cout << msg << "\n";
  ImguiOverlay::notify(std::move(msg));
}

void
AppViewer::toggle_tile_requests()
{
  bool const on = !ImageTiles::requests_enabled();
  ImageTiles::set_requests_enabled(on);
  std::string msg = std::string("Tile requests: ")
    + (on ? "ENABLED" : "DISABLED (cache-only)")
    + (on ? " — new provider jobs allowed" : " — new provider jobs suppressed");
  std::cout << msg << "\n";
  ImguiOverlay::notify(std::move(msg));
}

float
AppViewer::get_axis(SDL_GameController* gamecontroller, SDL_GameControllerAxis axis) const
{
  const float threshold = 0.002f; // was 0.25f; lower deadzone for finer stick control
  int value = SDL_GameControllerGetAxis(gamecontroller, axis);
  if (value == 0)
  {
    return 0.0f;
  }
  else if (value > 0)
  {
    return deadzone(static_cast<float>(value) / 32767.0f, threshold);
  }
  else // if (value < 0)
  {
    return deadzone(static_cast<float>(value) / 32768.0f, threshold);
  }
}

void
AppViewer::add_gamecontroller(int joy_id)
{
  SDL_GameController* gamecontroller = SDL_GameControllerOpen(joy_id);
  if (gamecontroller)
  {
    m_gamecontrollers.push_back(gamecontroller);
  }
}

void
AppViewer::remove_gamecontroller(int joy_id)
{
  auto gamecontroller_it = std::find_if(m_gamecontrollers.begin(), m_gamecontrollers.end(),
                                        [joy_id](SDL_GameController* gamecontroller)
                                        {
                                          SDL_Joystick* joy = SDL_GameControllerGetJoystick(gamecontroller);
                                          return (joy_id == SDL_JoystickInstanceID(joy));
                                        });
  assert(gamecontroller_it != m_gamecontrollers.end());
  SDL_GameControllerClose(*gamecontroller_it);
  m_gamecontrollers.erase(gamecontroller_it);
}

void
AppViewer::update_gamecontrollers(float delta)
{
  const auto move_x_axis = SDL_CONTROLLER_AXIS_LEFTX;
  const auto move_y_axis = SDL_CONTROLLER_AXIS_LEFTY;
  const auto zoom_axis   = SDL_CONTROLLER_AXIS_RIGHTY;
  const auto rotate_left_axis = SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
  const auto rotate_right_axis = SDL_CONTROLLER_AXIS_TRIGGERLEFT;

  for(SDL_GameController* gamecontroller: m_gamecontrollers)
  {
    float zoom_value = get_axis(gamecontroller, zoom_axis);
    float x_value = get_axis(gamecontroller, move_x_axis);
    float y_value = get_axis(gamecontroller, move_y_axis);
    float rotate_left_value = (get_axis(gamecontroller, rotate_left_axis) + 1.0f) / 2.0f * 180.0f;
    float rotate_right_value = (get_axis(gamecontroller, rotate_right_axis) + 1.0f) / 2.0f * 180.0f;

    // Response curve: more precision near center, stronger at edges
    x_value = std::copysign(std::pow(std::abs(x_value), 2.0f), x_value);
    y_value = std::copysign(std::pow(std::abs(y_value), 2.0f), y_value);

    { // zoom
      zoom_value *= 4.0f;

      if (zoom_value < 0.0f)
      {
        m_viewer.get_state().zoom(1.0f + (-zoom_value * delta));
      }
      else if (zoom_value > 0.0f)
      {
        m_viewer.get_state().zoom(1.0f / (1.0f + (zoom_value * delta)));
      }
    }

    { // move
      x_value *= 1524.0f; // was 2048.0f
      y_value *= 1524.0f;

      if (x_value != 0.0f || y_value != 0.0f)
      {
        m_viewer.get_state().move(Vector2f(x_value * delta,
                                           y_value * delta));
      }
    }

    { // rotate left
      rotate_left_value *= 2.0f;
      m_viewer.get_state().rotate(rotate_left_value * delta);
    }

    { // rotate right
      rotate_right_value *= 2.0f;
      m_viewer.get_state().rotate(-rotate_right_value * delta);
    }

    if (SDL_GameControllerGetButton(gamecontroller, SDL_CONTROLLER_BUTTON_BACK)) {
      m_viewer.get_state().set_angle(0.0f);
    }
  }
}

void
AppViewer::run()
{
  Uint32 ticks = SDL_GetTicks();

#ifdef HAVE_SPACE_NAVIGATOR
  SpaceNavigator space_navigator;
  space_navigator.start_thread();
#endif

  SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

  while(!m_quit)
  {
    if (m_viewer.is_active() || (true)) // FIXME: hack for joystick support // NOLINT
    {
      SDL_Event event;
      while (SDL_PollEvent(&event))
      {
        process_event(event);
      }

      Uint32 cticks = SDL_GetTicks();
      float delta = static_cast<float>(cticks - ticks) / 1000.0f;
      ticks = cticks;

      update_gamecontrollers(delta);

      m_viewer.update(delta);
      draw_frame();
    }
    else
    {
      SDL_Event event;
      SDL_WaitEvent(nullptr);
      while (SDL_PollEvent(&event))
      {
        process_event(event);
      }

      // FIXME: We should try to detect if we need a redraw and
      // only draw then, else we will redraw on each mouse motion
      draw_frame();
      ticks = SDL_GetTicks();
    }

    m_window->swap_buffers();
    // No fixed sleep: vsync (SetSwapInterval) paces the loop. A hard
    // SDL_Delay(10) capped throughput well below 60fps once draw work grew.
  }

#ifdef HAVE_SPACE_NAVIGATOR
  space_navigator.stop_thread();
#endif

  log_info("done");
}

} // namespace galapix

/* EOF */
