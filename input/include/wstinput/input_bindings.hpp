// SPDX-FileCopyrightText: 2005-2020 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_INPUT_INPUT_BINDINGS_HPP
#define HEADER_WINDSTILLE_INPUT_INPUT_BINDINGS_HPP

#include <filesystem>
#include <vector>

#include <SDL.h>

namespace wstinput {

class Controller;
class ControllerDescription;
class InputManagerSDL;

struct JoystickButtonBinding
{
  int event;
  int device;
  int button;
};

struct JoystickAxisBinding
{
  int  event;
  int  device;
  int  axis;
  bool invert;
};

struct JoystickButtonAxisBinding
{
  int event;
  int device;
  int minus;
  int plus;
};

struct JoystickAxisButtonBinding
{
  int  event;
  int  device;
  int  axis;
  bool up;
};

struct MouseButtonBinding
{
  int event;
  int device;
  int button;
};

struct MouseMotionBinding
{
  int event;
  int device;
  int axis;
};

struct MouseMotionBallBinding
{
  int event;
  int device;
  int axis;
};

struct KeyboardButtonBinding
{
  int event;
  SDL_Scancode key;
};

struct KeyboardAxisBinding
{
  int    event;
  SDL_Scancode minus;
  SDL_Scancode plus;
};

struct WiimoteButtonBinding
{
  int event;
  int device;
  int button;
};

struct WiimoteAxisBinding
{
  int event;
  int device;
  int axis;
};


class InputBindings
{
public:
  InputBindings(InputManagerSDL& manager);

  void load(std::filesystem::path const& filename,
            ControllerDescription const& controller_description);

  void bind_joystick_hat_axis(int event, int device, int axis);

  void bind_joystick_axis(int event, int device, int axis, bool invert);
  void bind_joystick_button_axis(int event, int device, int minus, int plus);
  void bind_joystick_button(int event, int device, int button);
  void bind_joystick_axis_button(int event, int device, int axis, bool up);

  void bind_keyboard_button(int event, SDL_Scancode key);
  void bind_keyboard_axis(int event, SDL_Scancode minus, SDL_Scancode plus);

  void bind_mouse_button(int event, int device, int button);
  void bind_mouse_motion(int event, int device, int axis);
  void bind_mouse_motion_ball(int event, int device, int axis);
  void bind_mouse_wheel(int event, int device, int wheel);

  void bind_wiimote_button(int event, int device, int button);
  void bind_wiimote_axis(int event, int device, int axis);

  void clear();

  /** Stick values with a smaller magnitude count as centered, in the
      range of SDL axis values (32767), the default is 8000 */
  void set_dead_zone(int dead_zone) { m_dead_zone = dead_zone; }
  int get_dead_zone() const { return m_dead_zone; }

  /** Keyboard, mouse and text events */
  void dispatch_event(SDL_Event const& event, Controller& controller) const;

  /** Joystick input by the device number used in the bindings, see
      InputManagerSDL for the button and axis layout */
  void dispatch_joystick_button(int device, int button, bool down, Controller& controller) const;
  void dispatch_joystick_axis(int device, int axis, int value, Controller& controller) const;

private:
  void dispatch_key_event(SDL_KeyboardEvent const& key, Controller& controller) const;
  void dispatch_mouse_button_event(SDL_MouseButtonEvent const& button, Controller& controller) const;
  void dispatch_mouse_motion_event(SDL_MouseMotionEvent const& motion, Controller& controller) const;
  void dispatch_mouse_wheel_event(SDL_MouseWheelEvent const& wheel, Controller& controller) const;

private:
  InputManagerSDL& m_manager;
  int m_dead_zone;

  std::vector<JoystickButtonBinding>     m_joystick_button_bindings;
  std::vector<JoystickButtonAxisBinding> m_joystick_button_axis_bindings;
  std::vector<JoystickAxisBinding>       m_joystick_axis_bindings;
  std::vector<JoystickAxisButtonBinding> m_joystick_axis_button_bindings;

  std::vector<KeyboardButtonBinding> m_keyboard_button_bindings;
  std::vector<KeyboardAxisBinding>   m_keyboard_axis_bindings;

  std::vector<MouseButtonBinding>   m_mouse_button_bindings;
  std::vector<MouseMotionBinding>   m_mouse_motion_bindings;
  std::vector<MouseMotionBallBinding>   m_mouse_motion_ball_bindings;

  std::vector<WiimoteButtonBinding> m_wiimote_button_bindings;
  std::vector<WiimoteAxisBinding>   m_wiimote_axis_bindings;

private:
  InputBindings(const InputBindings&) = delete;
  InputBindings& operator=(const InputBindings&) = delete;
};

} // namespace wstinput

#endif

/* EOF */
