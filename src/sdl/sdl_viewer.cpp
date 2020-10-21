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

#include "sdl/sdl_viewer.hpp"

#include <iostream>
#include <thread>
#include <fmt/format.h>
#include <logmich/log.hpp>
#include <SDL_keycode.h>

#include <surf/rgba.hpp>

#include "database/entries/old_file_entry.hpp"
#include "display/framebuffer.hpp"
#include "galapix/viewer.hpp"
#include "galapix/viewer_state.hpp"
#include "galapix/workspace.hpp"
#include "plugins/png.hpp"
#include "server/database_thread.hpp"
#include "spnav/space_navigator.hpp"
#include "util/filesystem.hpp"

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

SDLViewer::SDLViewer(const Size& geometry, bool fullscreen, int  anti_aliasing,
                     Viewer& viewer) :
  m_window(geometry, fullscreen, anti_aliasing),
  m_viewer(viewer),
  m_quit(false),
  m_spnav_allow_rotate(false),
  m_gamecontrollers()
{
}

SDLViewer::~SDLViewer()
{
  for(SDL_GameController* gamecontroller: m_gamecontrollers)
  {
    SDL_GameControllerClose(gamecontroller);
  }
}

void
SDLViewer::process_event(const SDL_Event& event)
{
  const Uint8* keystate = SDL_GetKeyboardState(nullptr);

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
                log_debug("MotionEvent: (%s, %s, %s) (%s, %s, %s)",
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

    case SDL_QUIT:
      std::cout << "Viewer: SDL_QUIT received" << std::endl;
      m_quit = true;
      break;

      //case SDL_VIDEOEXPOSE:
      //break;

    case SDL_WINDOWEVENT:
      switch (event.window.event)
      {
        case SDL_WINDOWEVENT_RESIZED:
          Framebuffer::reshape(Size(event.window.data1, event.window.data2));
          break;

        default:
          break;
      }
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
      for(int i = event.wheel.y; i < 0; ++i)
      {
        m_viewer.get_state().zoom(1.1f, Vector2i(event.button.x, event.button.y));
      }

      for(int i = event.wheel.y; i > 0; --i)
      {
        m_viewer.get_state().zoom(1.0f/1.1f, Vector2i(event.button.x, event.button.y));
      }
      break;

    case SDL_KEYDOWN:
      switch(event.key.keysym.sym)
      {
        case SDLK_ESCAPE:
          m_quit = true;
          break;

        case SDLK_d:
          m_viewer.zoom_to_selection();
          break;

        case SDLK_KP_PLUS:
          m_viewer.get_state().zoom(1.25f);
          break;

        case SDLK_KP_MINUS:
          m_viewer.get_state().zoom(1.0f/1.25f);
          break;

        case SDLK_KP_8:
          m_viewer.get_state().set_offset(m_viewer.get_state().get_offset().as_vec() + Vector2f(0.0f, +128.0f).as_vec());
          break;

        case SDLK_KP_2:
          m_viewer.get_state().set_offset(m_viewer.get_state().get_offset().as_vec() + Vector2f(0.0f, -128.0f).as_vec());
          break;

        case SDLK_KP_4:
          m_viewer.get_state().set_offset(m_viewer.get_state().get_offset().as_vec() + Vector2f(+128.0f, 0.0f).as_vec());
          break;

        case SDLK_KP_6:
          m_viewer.get_state().set_offset(m_viewer.get_state().get_offset().as_vec() + Vector2f(-128.0f, 0.0f).as_vec());
          break;

        case SDLK_p:
          m_viewer.set_pan_tool();
          break;

        case SDLK_r:
          m_viewer.set_move_rotate_tool();
          break;

        case SDLK_k:
          m_viewer.cleanup_cache();
          break;

        case SDLK_z:
          m_viewer.set_zoom_tool();
          break;

        case SDLK_m:
          m_viewer.set_move_resize_tool();
          break;

        case SDLK_h:
          m_viewer.zoom_home();
          break;

        case SDLK_s:
          if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT])
          {
            m_viewer.sort_reverse_image_list();
          }
          else
          {
            m_viewer.sort_image_list();
          }
          break;

        case SDLK_n:
          m_viewer.shuffle_image_list();
          break;

        case SDLK_F12:
          {
            SoftwareSurface surface = Framebuffer::screenshot();
            // FIXME: Could do this in a worker thread to avoid pause on screenshotting
            for(int i = 0; ; ++i)
            {
              std::string outfile = fmt::format("/tmp/galapix-screenshot-{:04d}.png", i);
              if (!Filesystem::exist(outfile))
              {
                png::save(surface, outfile);
                std::cout << "Screenshot written to " << outfile << std::endl;
                break;
              }
            }
          }
          break;

        case SDLK_i:
          m_viewer.isolate_selection();
          break;

        case SDLK_DELETE:
          m_viewer.delete_selection();
          break;

        case SDLK_y:
          m_viewer.set_grid_tool();
          break;

        case SDLK_F6:
          m_viewer.increase_brightness();
          break;

        case SDLK_F7:
          m_viewer.decrease_brightness();
          break;

        case SDLK_F8:
          m_viewer.increase_contrast();
          break;

        case SDLK_F9:
          m_viewer.decrease_contrast();
          break;

        case SDLK_F10:
          m_viewer.reset_gamma();
          break;

        case SDLK_PAGEUP:
          m_viewer.increase_gamma();
          break;

        case SDLK_PAGEDOWN:
          m_viewer.decrease_gamma();
          break;

        case SDLK_1:
          m_viewer.layout_auto();
          break;

        case SDLK_2:
          m_viewer.layout_tight();
          break;

        case SDLK_3:
          m_viewer.layout_random();
          break;

        case SDLK_4:
          m_viewer.layout_solve_overlaps();
          break;

        case SDLK_5:
          m_viewer.layout_spiral();
          break;

        case SDLK_6:
          m_viewer.layout_vertical();
          break;

        case SDLK_g:
          m_viewer.toggle_grid();
          break;

        case SDLK_f:
          m_viewer.toggle_pinned_grid();
          break;

        case SDLK_F11:
          m_window.toggle_fullscreen();
          break;

        case SDLK_F2:
          m_viewer.load();
          break;

        case SDLK_F3:
          m_viewer.save();
          break;

        case SDLK_c:
          m_viewer.clear_cache();
          break;

        case SDLK_F5:
          m_viewer.refresh_selection();
          break;

        case SDLK_t:
          m_viewer.toggle_trackball_mode();
          break;

        case SDLK_UP:
        case SDLK_DOWN:
          m_viewer.reset_view_rotation();
          break;

        case SDLK_LEFT:
          m_viewer.rotate_view_270();
          break;

        case SDLK_RIGHT:
          m_viewer.rotate_view_90();
          break;

        case SDLK_b:
          if (keystate[SDL_SCANCODE_LSHIFT] || keystate[SDL_SCANCODE_RSHIFT])
          {
            m_viewer.toggle_background_color(true);
          }
          else
          {
            m_viewer.toggle_background_color();
          }
          break;

        case SDLK_SPACE:
          m_viewer.print_images();
          break;

        case SDLK_l:
          m_viewer.print_state();
          break;

        case SDLK_0:
          m_viewer.print_info();
          break;

        default:
          m_viewer.on_key_down(sdlkey2viewer(event.key.keysym.sym));
          break;
      }
      break;

    case SDL_KEYUP:
      m_viewer.on_key_up(sdlkey2viewer(event.key.keysym.sym));
      break;

    default:
      break;
  }
}

float
SDLViewer::get_axis(SDL_GameController* gamecontroller, SDL_GameControllerAxis axis) const
{
  int value = SDL_GameControllerGetAxis(gamecontroller, axis);
  if (value == 0)
  {
    return 0.0f;
  }
  else if (value > 0)
  {
    return deadzone(static_cast<float>(value) / 32767.0f, 0.25f);
  }
  else // if (value < 0)
  {
    return deadzone(static_cast<float>(value) / 32768.0f, 0.25f);
  }
}

void
SDLViewer::add_gamecontroller(int joy_id)
{
  SDL_GameController* gamecontroller = SDL_GameControllerOpen(joy_id);
  if (gamecontroller)
  {
    m_gamecontrollers.push_back(gamecontroller);
  }
}

void
SDLViewer::remove_gamecontroller(int joy_id)
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
SDLViewer::update_gamecontrollers(float delta)
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
      x_value *= 2048.0f;
      y_value *= 2048.0f;

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
  }
}

void
SDLViewer::run()
{
  Uint32 ticks = SDL_GetTicks();

#ifdef HAVE_SPACE_NAVIGATOR
  SpaceNavigator space_navigator;
  space_navigator.start_thread();
#endif

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
      m_viewer.draw();
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
      m_viewer.draw();
      ticks = SDL_GetTicks();
    }

    m_window.flip();

    // std::cout << "." << std::flush;

    // FIXME: Higher values mean less CPU use for drawing and more
    // for loading, which is nice for low-end CPUs, but not so nice
    // for high-end CPUs, maybe make this configurable
    SDL_Delay(10);
  }

#ifdef HAVE_SPACE_NAVIGATOR
  space_navigator.stop_thread();
#endif

  log_info("done");
}

/* EOF */
