// SPDX-FileCopyrightText: 2005-2020 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WINDSTILLE_INPUT_INPUT_EVENT_HPP
#define HEADER_WINDSTILLE_INPUT_INPUT_EVENT_HPP

#include <array>
#include <vector>

#include <SDL.h>

namespace wstinput {

enum InputEventType
{
  BUTTON_EVENT,
  AXIS_EVENT,
  BALL_EVENT,
  POINTER_EVENT,
  TEXT_EVENT,
  TEXT_EDIT_EVENT,
  KEYBOARD_EVENT
};

/** Used for textual input */
struct TextEvent
{
  std::array<char, 32> text;
};

struct TextEditEvent
{
  std::array<char, 32> text;
  int start;
  int length;
};

/** Raw keyboard events, only send when text input is active */
struct KeyboardEvent
{
  SDL_KeyboardEvent key;
};

struct ButtonEvent
{
  int name;

  /** true if down, false if up */
  bool down;

  bool is_down() const { return down; }
  bool is_up()   const { return !down; }
};

struct BallEvent
{
  int   name;
  float pos;
  float get_pos() const { return pos; }
};

struct PointerEvent
{
  int   name;
  float pos;
  float get_pos() const { return pos; }
};

struct AxisEvent
{
  int name;

  /** Pos can be in range from [-1.0, 1.0], some axis will only use [0,1.0] */
  float pos;

  float get_pos() const { return pos; }
};

struct InputEvent
{
  InputEventType type;

  union
  {
    struct ButtonEvent button;
    struct AxisEvent axis;
    struct TextEvent text;
    struct TextEditEvent text_edit;
    struct KeyboardEvent keyboard;
    struct BallEvent ball;
  };
};

using InputEventLst = std::vector<InputEvent>;

} // namespace wstinput

#endif

/* EOF */
