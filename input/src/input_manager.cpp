// SPDX-FileCopyrightText: 2005-2020 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <algorithm>
#include <sstream>

#include <logmich/log.hpp>
#include <prio/reader.hpp>

#include <wstinput/input_manager.hpp>
#ifdef HAVE_CWIID
#  include "wiimote.hpp"
#endif

using namespace prio;

namespace wstinput {

InputManagerSDL::InputManagerSDL(ControllerDescription const& controller_description) :
  m_controller_description(controller_description),
  m_controller(controller_description.get_max_id() + 1),
  m_bindings(*this),
  m_pads(),
  m_keyidmapping()
{
  if (SDL_InitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) < 0) {
    log_error("InputManagerSDL: couldn't initialize joystick support: {}", SDL_GetError());
  }

  log_debug("Keyboard keys:");
  for (int i = 0; i < SDL_NUM_SCANCODES; ++i) {
    const char* key_name = SDL_GetScancodeName(static_cast<SDL_Scancode>(i));
    m_keyidmapping[key_name] = static_cast<SDL_Scancode>(i);
    // FIXME: Make the keynames somewhere user visible so that users can use them
    log_debug("  {}", key_name);
  }

  stop_text_input();

  // pads attached later are opened on SDL_JOYDEVICEADDED
  for (int index = 0; index < SDL_NumJoysticks(); ++index) {
    open_pad(index);
  }

#ifdef HAVE_CWIID
  // FIXME: doesn't really belong here
  Wiimote::init();

  if (wiimote && config.get<bool>("wiimote").get())
    wiimote->connect();

#endif // HAVE_CWIID
}

InputManagerSDL::~InputManagerSDL()
{
  for (Pad& pad : m_pads) {
    close_pad(pad);
  }
  SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER);

#ifdef HAVE_CWIID
  Wiimote::deinit();
#endif
}

void
InputManagerSDL::load(std::filesystem::path const& filename)
{
  m_bindings.load(filename, m_controller_description);
}

std::string
InputManagerSDL::keyid_to_string(SDL_Scancode id) const
{
  return SDL_GetKeyName(id);
}

SDL_Scancode
InputManagerSDL::string_to_keyid(const std::string& str) const
{
  auto const it = m_keyidmapping.find(str);
  if (it == m_keyidmapping.end())
  {
    std::ostringstream msg;
    msg << "key lookup failure for '" << str << "'";
    throw std::runtime_error(msg.str());
  }
  else
  {
    return it->second;
  }
}

void
InputManagerSDL::open_pad(int index)
{
  if (find_device(SDL_JoystickGetDeviceInstanceID(index)) >= 0) {
    return;
  }

  auto it = std::find_if(m_pads.begin(), m_pads.end(), [](Pad const& pad) { return !pad.joystick; });
  if (it == m_pads.end()) {
    it = m_pads.insert(m_pads.end(), Pad{});
  }
  int const device = static_cast<int>(it - m_pads.begin());
  Pad& pad = *it;

  if (SDL_IsGameController(index)) {
    if (SDL_GameController* controller = SDL_GameControllerOpen(index)) {
      pad.controller = controller;
      pad.joystick = SDL_GameControllerGetJoystick(controller);
      pad.instance_id = SDL_JoystickInstanceID(pad.joystick);
      log_info("InputManagerSDL: game controller {}: {}", device, SDL_GameControllerName(controller));
      return;
    }
  }

  if (SDL_Joystick* joystick = SDL_JoystickOpen(index)) {
    pad.joystick = joystick;
    pad.instance_id = SDL_JoystickInstanceID(joystick);
    log_info("InputManagerSDL: joystick {}: {}", device, SDL_JoystickName(joystick));
  } else {
    log_error("InputManagerSDL: couldn't open joystick {}: {}", index, SDL_GetError());
  }
}

void
InputManagerSDL::close_pad(Pad& pad)
{
  if (pad.controller) {
    SDL_GameControllerClose(pad.controller);
  } else if (pad.joystick) {
    SDL_JoystickClose(pad.joystick);
  }
  pad = Pad{};
}

int
InputManagerSDL::find_device(SDL_JoystickID instance_id) const
{
  for (size_t i = 0; i < m_pads.size(); ++i) {
    if (m_pads[i].joystick && m_pads[i].instance_id == instance_id) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

void
InputManagerSDL::dispatch_hat(int device, Uint8 value)
{
  Pad& pad = m_pads[static_cast<size_t>(device)];

  // the same button numbers as the D-pad of a game controller
  constexpr std::pair<Uint8, int> directions[] = {
    {SDL_HAT_UP, SDL_CONTROLLER_BUTTON_DPAD_UP},
    {SDL_HAT_DOWN, SDL_CONTROLLER_BUTTON_DPAD_DOWN},
    {SDL_HAT_LEFT, SDL_CONTROLLER_BUTTON_DPAD_LEFT},
    {SDL_HAT_RIGHT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT},
  };
  for (auto const& [mask, button] : directions) {
    bool const down = (value & mask) != 0;
    if (down != ((pad.hat & mask) != 0)) {
      m_bindings.dispatch_joystick_button(device, button, down, m_controller);
    }
  }
  pad.hat = value;
}

void
InputManagerSDL::on_event(const SDL_Event& event)
{
  switch (event.type)
  {
    case SDL_JOYDEVICEADDED:
      // 'which' is the device index here, a controller sends this too
      open_pad(event.jdevice.which);
      break;

    case SDL_JOYDEVICEREMOVED: {
      // 'which' is the instance id, a controller sends both removed events
      int const device = find_device(event.jdevice.which);
      if (device >= 0) {
        log_info("InputManagerSDL: joystick {} removed", device);
        close_pad(m_pads[static_cast<size_t>(device)]);
      }
      break;
    }

    // a pad opened as game controller also sends JOY* events, they
    // are only used for plain joysticks so each input counts once

    case SDL_JOYAXISMOTION: {
      int const device = find_device(event.jaxis.which);
      if (device >= 0 && !m_pads[static_cast<size_t>(device)].controller) {
        m_bindings.dispatch_joystick_axis(device, event.jaxis.axis, event.jaxis.value, m_controller);
      }
      break;
    }

    case SDL_JOYBUTTONDOWN:
    case SDL_JOYBUTTONUP: {
      int const device = find_device(event.jbutton.which);
      if (device >= 0 && !m_pads[static_cast<size_t>(device)].controller) {
        m_bindings.dispatch_joystick_button(device, event.jbutton.button,
                                            event.jbutton.state == SDL_PRESSED, m_controller);
      }
      break;
    }

    case SDL_JOYHATMOTION: {
      int const device = find_device(event.jhat.which);
      if (device >= 0 && !m_pads[static_cast<size_t>(device)].controller && event.jhat.hat == 0) {
        dispatch_hat(device, event.jhat.value);
      }
      break;
    }

    case SDL_CONTROLLERAXISMOTION: {
      int const device = find_device(event.caxis.which);
      if (device >= 0) {
        m_bindings.dispatch_joystick_axis(device, event.caxis.axis, event.caxis.value, m_controller);
      }
      break;
    }

    case SDL_CONTROLLERBUTTONDOWN:
    case SDL_CONTROLLERBUTTONUP: {
      int const device = find_device(event.cbutton.which);
      if (device >= 0) {
        m_bindings.dispatch_joystick_button(device, event.cbutton.button,
                                            event.cbutton.state == SDL_PRESSED, m_controller);
      }
      break;
    }

    default:
      m_bindings.dispatch_event(event, m_controller);
      break;
  }
}

void
InputManagerSDL::update(float /*delta*/)
{
#ifdef HAVE_CWIID
  if (wiimote && wiimote->is_connected())
  {
    // Check for new events from the Wiimote
    std::vector<WiimoteEvent> events = wiimote->pop_events();
    for(std::vector<WiimoteEvent>::iterator i = events.begin(); i != events.end(); ++i)
    {
      WiimoteEvent& event = *i;
      if (event.type == WiimoteEvent::WIIMOTE_BUTTON_EVENT)
      {
        for (std::vector<WiimoteButtonBinding>::const_iterator j = m_wiimote_button_bindings.begin();
             j != m_wiimote_button_bindings.end();
             ++j)
        {
          if (event.button.device == j->device &&
              event.button.button == j->button)
          {
            add_button_event(j->event, event.button.down);
          }
        }
      }
      else if (event.type == WiimoteEvent::WIIMOTE_AXIS_EVENT)
      {
        for (std::vector<WiimoteAxisBinding>::const_iterator j = m_wiimote_axis_bindings.begin();
             j != m_wiimote_axis_bindings.end();
             ++j)
        {
          if (event.axis.device == j->device &&
              event.axis.axis == j->axis)
          {
            add_axis_event(j->event, event.axis.pos);
          }
        }
      }
      else if (event.type == WiimoteEvent::WIIMOTE_ACC_EVENT)
      {
        if (event.acc.accelerometer == 0)
        {
          if (0)
            printf("%d - %6.3f %6.3f %6.3f\n",
                   event.acc.accelerometer,
                   event.acc.x,
                   event.acc.y,
                   event.acc.z);

          float roll = atanf(static_cast<float>(event.acc.x / event.acc.z));
          if (event.acc.z <= 0.0) {
            roll += math::pi * ((event.acc.x > 0.0f) ? 1.0f : -1.0f);
          }
          roll *= -1;

          float pitch = atanf(event.acc.y / event.acc.z * cosf(roll));

          add_axis_event(X2_AXIS, math::mid(-1.0f, -float(pitch / M_PI), 1.0f));
          add_axis_event(Y2_AXIS, math::mid(-1.0f, -float(roll  / M_PI), 1.0f));

          log_debug("{:6.3f} {:6.3f}", pitch, roll);
        }
      }
      else
      {
        assert(!"Never reached");
      }
    }
  }
#endif
}

void
InputManagerSDL::clear()
{
  m_controller.clear();
}

void
InputManagerSDL::start_text_input()
{
  SDL_StartTextInput();
}

void
InputManagerSDL::stop_text_input()
{
  SDL_StopTextInput();
}

bool
InputManagerSDL::is_text_input_active() const
{
  return SDL_IsTextInputActive();
}

} // namespace wstinput

/* EOF */
