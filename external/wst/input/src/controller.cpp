// SPDX-FileCopyrightText: 2005-2020 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstinput/controller.hpp>

#include <math.h>
#include <assert.h>

namespace wstinput {

Controller::Controller(size_t size) :
  m_states(size),
  m_events() // FIXME: need to mark states with type
{
}

float
Controller::get_trigger_state(int name) const
{
  if (m_states.empty()) { return 0.0f; }

  float value = get_axis_state(name)/2.0f + 0.5f;
  if (value < 0.001f)
  {
    return 0;
  }
  else if (value > 0.999f)
  {
    return 1.0f;
  }
  else
  {
    return value;
  }
}

float
Controller::get_axis_state(int id, bool use_deadzone) const
{
  if (m_states.empty()) { return 0.0f; }

  assert(id < int(m_states.size()));

  if (use_deadzone)
  {
    if (fabsf(m_states[id].axis) > 0.25f) // FIXME: Hardcoded Deadzone
      return m_states[id].axis;
    else
      return 0.0f;
  }
  else
  {
    return m_states[id].axis;
  }
}

bool
Controller::get_button_state(int id) const
{
  if (m_states.empty()) { return false; }

  assert(id < int(m_states.size()));
  return m_states[id].button;
}

void
Controller::set_axis_state(int id, float pos)
{
  if (m_states.empty()) { return; }

  assert(id < static_cast<int>(m_states.size()));
  m_states[id].axis = pos;
}

void
Controller::set_button_state(int name, bool down)
{
  if (m_states.empty()) { return; }

  assert(name < static_cast<int>(m_states.size()));
  m_states[name].button = down;
}

const InputEventLst&
Controller::get_events() const
{
  return m_events;
}

bool
Controller::button_was_pressed(int name) const
{
  for(InputEventLst::const_iterator i = m_events.begin(); i != m_events.end(); ++i)
  {
    if (i->type == BUTTON_EVENT && i->button.name == name && i->button.down)
    {
      return true;
    }
  }
  return false;
}

bool
Controller::axis_was_pressed_up(int name) const
{
  for(InputEventLst::const_iterator i = m_events.begin(); i != m_events.end(); ++i)
  {
    if (i->type == AXIS_EVENT && i->axis.name == name && i->axis.pos > 0.5f)
    {
      return true;
    }
  }
  return false;
}

bool
Controller::axis_was_pressed_down(int name) const
{
  for(InputEventLst::const_iterator i = m_events.begin(); i != m_events.end(); ++i)
  {
    if (i->type == AXIS_EVENT && i->axis.name == name && i->axis.pos < -0.5f)
    {
      return true;
    }
  }
  return false;
}

void
Controller::clear()
{
  m_events.clear();
}

void
Controller::add_event(const InputEvent& event)
{
  m_events.push_back(event);
}

float
Controller::get_ball_state(int id) const
{
  if (m_states.empty()) { return 0.0f; }

  assert(id < int(m_states.size()));
  return m_states[id].ball;
}

float
Controller::get_pointer_state(int id) const
{
  if (m_states.empty()) { return 0.0f; }

  assert(id < int(m_states.size()));
  return m_states[id].pointer;
}

void
Controller::set_ball_state(int id, float pos)
{
  assert(id < static_cast<int>(m_states.size()));
  m_states[id].ball = pos;
}

void
Controller::set_pointer_state(int id, float pos)
{
  assert(id < static_cast<int>(m_states.size()));
  m_states[id].pointer = pos;
}

void
Controller::add_ball_event(int name, float pos)
{
  InputEvent event;

  event.type = BALL_EVENT;
  event.axis.name = name;
  event.axis.pos  = pos;

  add_event(event);
  set_ball_state(name, pos);
}

void
Controller::add_pointer_event(int name, float pos)
{
  InputEvent event;

  event.type = POINTER_EVENT;
  event.axis.name = name;
  event.axis.pos  = pos;

  add_event(event);
  set_pointer_state(name, pos);
}

void
Controller::add_button_event(int name, bool down)
{
  InputEvent event;

  event.type = BUTTON_EVENT;
  event.button.name = name;
  event.button.down = down;

  add_event(event);
  set_button_state(name, down);
}

void
Controller::add_text_event(int , std::array<char, 32> const& text)
{
  InputEvent event;

  event.type = TEXT_EVENT;
  event.text.text = text;

  add_event(event);
}

void
Controller::add_text_edit_event(int , std::array<char, 32> const& text, int start, int length)
{
  InputEvent event;

  event.type = TEXT_EVENT;
  event.text_edit.text = text;
  event.text_edit.start = start;
  event.text_edit.length = length;

  add_event(event);
}

void
Controller::add_keyboard_event(SDL_KeyboardEvent const& key)
{
  InputEvent event;

  event.type = KEYBOARD_EVENT;
  event.keyboard.key = key;

  add_event(event);
}

void
Controller::add_axis_event(int name, float pos)
{
#if 0
  // FIXME: reimplement this in some generic fashion

  // Convert analog axis events into digital menu movements
  // FIXME: add key repeat
  float click_threshold = 0.5f;
  float release_threshold = 0.3f;

  // FIXME: need state info
  float old_pos = m_controller.get_axis_state(name);
  if (name == X_AXIS)
  {
    if (m_controller.get_button_state(MENU_LEFT_BUTTON) == 0 &&
        pos < -click_threshold && old_pos > -click_threshold)
    {
      add_button_event(MENU_LEFT_BUTTON, 1);
    }
    else if (m_controller.get_button_state(MENU_LEFT_BUTTON) == 1 &&
             old_pos < -release_threshold && pos > -release_threshold)
    {
      add_button_event(MENU_LEFT_BUTTON, 0);
    }

    else if (m_controller.get_button_state(MENU_RIGHT_BUTTON) == 0 &&
             pos > click_threshold && old_pos < click_threshold)
    {
      add_button_event(MENU_RIGHT_BUTTON, 1);
    }
    else  if (m_controller.get_button_state(MENU_RIGHT_BUTTON) == 1 &&
              old_pos > release_threshold && pos < release_threshold)
    {
      add_button_event(MENU_RIGHT_BUTTON, 0);
    }
  }
  else if (name == Y_AXIS)
  {
    if (m_controller.get_button_state(MENU_UP_BUTTON) == 0 &&
        pos < -click_threshold && old_pos > -click_threshold)
    {
      add_button_event(MENU_UP_BUTTON, 1);
    }
    else if (m_controller.get_button_state(MENU_UP_BUTTON) == 1 &&
             old_pos < -release_threshold && pos > -release_threshold)
    {
      add_button_event(MENU_UP_BUTTON, 0);
    }

    else  if (m_controller.get_button_state(MENU_DOWN_BUTTON) == 0 &&
              pos > click_threshold && old_pos < click_threshold)
    {
      add_button_event(MENU_DOWN_BUTTON, 1);
    }
    else  if (m_controller.get_button_state(MENU_DOWN_BUTTON) == 1 &&
              old_pos > release_threshold && pos < release_threshold)
    {
      add_button_event(MENU_DOWN_BUTTON, 0);
    }
  }
#endif

  InputEvent event;

  event.type = AXIS_EVENT;
  event.axis.name = name;
  event.axis.pos  = pos;

  add_event(event);
  set_axis_state(name, pos);
}

} // namespace wstinput

/* EOF */
