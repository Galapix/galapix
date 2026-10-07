// SPDX-FileCopyrightText: 2005-2020 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_INPUT_INPUT_MANAGER_HPP
#define HEADER_WINDSTILLE_INPUT_INPUT_MANAGER_HPP

#include <SDL.h>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <prio/fwd.hpp>

#include "controller.hpp"
#include "controller_description.hpp"
#include "input_bindings.hpp"

namespace wstinput {

class InputManagerSDLImpl;

class InputManagerSDL
{
public:
  InputManagerSDL(ControllerDescription const& controller_description);
  ~InputManagerSDL();

  void load(std::filesystem::path const& filename);

  void update(float delta);

  void clear();

  std::string keyid_to_string(SDL_Scancode id) const;
  SDL_Scancode string_to_keyid(const std::string& str) const;

  ControllerDescription const& get_controller_description() const { return m_controller_description; }
  Controller const& get_controller() const { return m_controller; }

  /** For injecting input from other sources, e.g. an on-screen gamepad */
  Controller& get_controller() { return m_controller; }

  InputBindings& bindings() { return m_bindings; }

  void on_event(const SDL_Event& event);

  void start_text_input();
  void stop_text_input();
  bool is_text_input_active() const;

private:
  ControllerDescription m_controller_description;
  Controller m_controller;
  InputBindings m_bindings;
  struct Pad
  {
    /** Set when opened as a game controller, owns the joystick then */
    SDL_GameController* controller = nullptr;
    SDL_Joystick* joystick = nullptr;
    SDL_JoystickID instance_id = -1;
    /** Last hat value, to report only the D-pad buttons that changed */
    Uint8 hat = SDL_HAT_CENTERED;
  };

  /** Open the SDL device \a index in the first free slot, unless it
      is open already */
  void open_pad(int index);
  void close_pad(Pad& pad);

  /** The device number of the pad with \a instance_id, -1 if unknown */
  int find_device(SDL_JoystickID instance_id) const;

  /** Map a hat of a plain joystick onto the D-pad buttons */
  void dispatch_hat(int device, Uint8 value);

  /** Indexed by the device number used in the bindings. Pads present
      at startup get the SDL order, pads plugged in later the first
      free slot, so the numbers of the others never change. Devices
      that SDL knows as game controllers are opened with the
      GameController API, they have the standard layout: buttons A=0,
      B=1, X=2, Y=3, ..., DPAD_UP=11, DOWN=12, LEFT=13, RIGHT=14 and
      axes LEFTX=0, LEFTY=1, RIGHTX=2, RIGHTY=3, triggers 4 and 5.
      The first hat of a plain joystick is reported as the D-pad
      buttons. */
  std::vector<Pad> m_pads;
  std::map<std::string, SDL_Scancode> m_keyidmapping;

private:
  InputManagerSDL (const InputManagerSDL&);
  InputManagerSDL& operator= (const InputManagerSDL&);
};

} // namespace wstinput

#endif

/* EOF */
