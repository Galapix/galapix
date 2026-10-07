// SPDX-FileCopyrightText: 2005-2020 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <stdexcept>

#include <wstinput/controller_description.hpp>

namespace wstinput {

ControllerDescription::ControllerDescription()
  : str_to_event(),
    id_to_event()
{
}

ControllerDescription::~ControllerDescription()
{
}

void
ControllerDescription::add_button(const std::string& name, int id)
{
  InputEventDefinition event;

  event.type = BUTTON_EVENT;
  event.name = name;
  event.id   = id;

  str_to_event[event.name] = event;
  id_to_event[event.id]    = event;
}

void
ControllerDescription::add_pointer(const std::string& name, int id)
{
  InputEventDefinition event;

  event.type = POINTER_EVENT;
  event.name = name;
  event.id   = id;

  str_to_event[event.name] = event;
  id_to_event[event.id]    = event;
}

void
ControllerDescription::add_ball(const std::string& name, int id)
{
  InputEventDefinition event;

  event.type = BALL_EVENT;
  event.name = name;
  event.id   = id;

  str_to_event[event.name] = event;
  id_to_event[event.id]    = event;
}

void
ControllerDescription::add_axis(const std::string& name, int id)
{
  InputEventDefinition event;

  event.type = AXIS_EVENT;
  event.name = name;
  event.id   = id;

  str_to_event[event.name] = event;
  id_to_event[event.id]    = event;
}

const InputEventDefinition&
ControllerDescription::get_definition(int id) const
{
  std::map<int, InputEventDefinition>::const_iterator i = id_to_event.find(id);
  if (i == id_to_event.end())
    throw std::runtime_error("Unknown event id");

  return i->second;
}

const InputEventDefinition&
ControllerDescription::get_definition(const std::string& name) const
{
  std::map<std::string, InputEventDefinition>::const_iterator i = str_to_event.find(name);
  if (i == str_to_event.end())
    throw std::runtime_error("Unknown event str: " + name);

  return i->second;
}

int
ControllerDescription::get_max_id() const
{
  int result = 0;
  for(auto const& item : id_to_event) {
    result = std::max(result, item.first);
  }
  return result;
}

} // namespace wstinput

/* EOF */
