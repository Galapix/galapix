// SPDX-FileCopyrightText: 2018 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <SDL.h>

#include <iostream>
#include <sstream>

#include <logmich/log.hpp>

#include <wstdisplay/opengl_window.hpp>

#include <wstsystem/system.hpp>

namespace wstsystem {

System::System() :
  sig_keyboard_event(),
  sig_text_input_event(),
  sig_text_editing_event(),
  sig_mouse_button_event(),
  sig_mouse_motion_event(),
  sig_mouse_wheel_event(),
  m_quit(false),
  m_windows(),
  m_sig_event()
{
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    std::ostringstream os;
    os << "Couldn't initialize SDL: " << SDL_GetError();
    throw std::runtime_error(os.str());
  }
  atexit(SDL_Quit);
}

System::~System()
{
  SDL_Quit();
}

std::unique_ptr<wstdisplay::OpenGLWindow>
System::create_window(wstdisplay::OpenGLWindow::Params const& params)
{
  auto window = std::make_unique<wstdisplay::OpenGLWindow>(*this, params);

  m_windows[window->get_id()] = window.get();
  return window;
}

std::unique_ptr<wstdisplay::OpenGLWindow>
System::create_window(std::string title, geom::isize const& size)
{
  return create_window({
      .title = std::move(title),
      .icon = {},
      .size = size,
      .resizable = true
   });
}

void
System::run()
{
  m_quit = false;
  while (!m_quit)
  {
    update();
  }
}

void
System::update()
{
  SDL_Event event;
  while (SDL_PollEvent(&event))
  {
    m_sig_event(event);

    // https://wiki.libsdl.org/SDL_EventType
    switch(event.type)
    {
      case SDL_QUIT:
        // FIXME: This should be a bit more gentle, but will do for now
        m_quit = true;
        break;

      case SDL_WINDOWEVENT:
        {
          auto it = m_windows.find(event.window.windowID);
          if (it != m_windows.end()) {
            it->second->handle_event(event.window);
          }
        }
        break;

      case SDL_SYSWMEVENT:
        break;

      case SDL_KEYUP:
      case SDL_KEYDOWN:
        sig_keyboard_event(event.key);
        break;

      case SDL_TEXTEDITING:
        sig_text_editing_event(event.edit);
        break;

      case SDL_TEXTINPUT:
        sig_text_input_event(event.text);
        break;

      case SDL_KEYMAPCHANGED:
        break;

      case SDL_MOUSEMOTION:
        sig_mouse_motion_event(event.motion);
        break;

      case SDL_MOUSEBUTTONUP:
      case SDL_MOUSEBUTTONDOWN:
        sig_mouse_button_event(event.button);
        break;

      case SDL_JOYDEVICEADDED:
      case SDL_JOYDEVICEREMOVED:
      case SDL_JOYAXISMOTION:
      case SDL_JOYBALLMOTION:
      case SDL_JOYHATMOTION:
      case SDL_JOYBUTTONUP:
      case SDL_JOYBUTTONDOWN:
        break;

      case SDL_CONTROLLERDEVICEREMOVED:
      case SDL_CONTROLLERDEVICEREMAPPED:
      case SDL_CONTROLLERAXISMOTION:
      case SDL_CONTROLLERBUTTONUP:
      case SDL_CONTROLLERBUTTONDOWN:
      case SDL_CONTROLLERDEVICEADDED:
        break;

      case SDL_CLIPBOARDUPDATE:
        break;

      case SDL_DROPFILE:
      case SDL_DROPTEXT:
      case SDL_DROPBEGIN:
      case SDL_DROPCOMPLETE:
        break;

      case SDL_AUDIODEVICEADDED:
      case SDL_AUDIODEVICEREMOVED:
        break;

      case SDL_RENDER_TARGETS_RESET:
      case SDL_RENDER_DEVICE_RESET:
        break;

      case SDL_USEREVENT:
        break;

      default:
        log_debug("System: unhandled event: {}", event.type);
        break;
    }
  }
}

int
System::get_ticks()
{
  return SDL_GetTicks();
}

void
System::delay(int msec)
{
  SDL_Delay(msec);
}

void
System::remove_window(uint32_t id)
{
  m_windows.erase(id);
}

void
System::quit()
{
  m_quit = true;
}

} // namespace wstsystem

/* EOF */
