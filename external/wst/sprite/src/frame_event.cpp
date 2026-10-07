// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstsprite/frame_event.hpp>

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace wstsprite {

EventRegistry::EventRegistry() :
  m_entries()
{
}

EventType
EventRegistry::add(std::string_view name)
{
  return add_parser(name, {});
}

EventType
EventRegistry::add_parser(std::string_view name, Parser parser)
{
  if (find(name) != EventType::Unknown) {
    throw std::invalid_argument("EventRegistry: event already registered: " + std::string(name));
  }

  if (m_entries.size() >= std::numeric_limits<uint16_t>::max()) {
    throw std::length_error("EventRegistry: too many event types");
  }

  m_entries.push_back(Entry{std::string(name), std::move(parser)});
  return static_cast<EventType>(m_entries.size());
}

EventType
EventRegistry::find(std::string_view name) const
{
  auto it = std::find_if(m_entries.begin(), m_entries.end(),
                         [name](Entry const& entry) { return entry.name == name; });
  if (it == m_entries.end()) {
    return EventType::Unknown;
  }
  return static_cast<EventType>(it - m_entries.begin() + 1);
}

std::string_view
EventRegistry::get_name(EventType type) const
{
  auto const index = static_cast<size_t>(type);
  if (index == 0 || index > m_entries.size()) {
    return {};
  }
  return m_entries[index - 1].name;
}

void
EventRegistry::resolve(FrameEvent& event) const
{
  event.type = find(event.name);
  if (event.type == EventType::Unknown) {
    return;
  }

  Entry const& entry = m_entries[static_cast<size_t>(event.type) - 1];
  if (entry.parser) {
    try {
      event.data = entry.parser(event.properties);
    } catch (std::exception const& err) {
      throw std::runtime_error("event '" + event.name + "' in frame " + std::to_string(event.frame) +
                               ": " + err.what());
    }
  }
}

} // namespace wstsprite

/* EOF */
