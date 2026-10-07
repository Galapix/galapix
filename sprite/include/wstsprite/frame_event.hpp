// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTSPRITE_FRAME_EVENT_HPP
#define HEADER_WSTSPRITE_FRAME_EVENT_HPP

#include <any>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "properties.hpp"

namespace wstsprite {

/** The type of a FrameEvent, assigned by an EventRegistry. Compare
    against the value EventRegistry::add() returned. */
enum class EventType : uint16_t
{
  /** Not registered, only name and properties are available */
  Unknown = 0
};

/** Something a game wants to happen when an animation reaches a frame,
    e.g. (sound (frame 7) (name "splash") (volume 0.5)). The element
    name is the event name, the library only stores and reports
    events, the game decides what they do.

    With an EventRegistry the event is resolved while loading: type
    identifies it without string compares and data holds whatever the
    registered parser made of the properties, so nothing needs to be
    parsed when the event fires. */
struct FrameEvent
{
  int frame = 0;
  EventType type = EventType::Unknown;
  std::string name = {};
  Properties properties = {};
  std::any data = {};

  /** The data produced by the registered parser, nullptr if there is
      none or it is not a T */
  template<typename T>
  T const* get() const { return std::any_cast<T>(&data); }
};

/** A fired event as reported by advance(): the index of the action and
    of the event in Action::events. A plain value that stays meaningful
    when queued, look the event up with SpriteData::get_event(). */
struct EventRef
{
  int action = 0;
  int event = 0;

  friend bool operator==(EventRef const& lhs, EventRef const& rhs) = default;
};

static_assert(std::is_trivially_copyable_v<EventRef>);

/** Maps event names to EventTypes and parsers that turn the event
    properties into game specific data at load time:

      struct SoundEvent { SoundId sound; float volume; };

      EventRegistry events;
      EventType const sound = events.add("sound", [&](Properties const& props) {
        return SoundEvent{sounds.load(props.get_string("name").value()),
                          props.get_float("volume").value_or(1.0f)};
      });

      SpriteManager sprites(surfaces, &events);

      // when advance() reports events
      FrameEvent const& event = data.get_event(ref);
      if (event.type == sound) {
        SoundEvent const& s = *event.get<SoundEvent>();
        play(s.sound, s.volume);
      }

    A parser that throws makes loading the sprite fail with the error
    message. */
class EventRegistry final
{
public:
  using Parser = std::function<std::any (Properties const&)>;

public:
  EventRegistry();

  /** Register \a name without data, throws if it is already registered */
  EventType add(std::string_view name);

  /** Register \a name with \a parser, a callable taking Properties
      const& and returning the data, which ends up in FrameEvent::data */
  template<typename F>
  EventType add(std::string_view name, F parser)
  {
    using T = std::invoke_result_t<F&, Properties const&>;
    static_assert(!std::is_void_v<T>, "the parser must return the event data");
    return add_parser(name, [parser = std::move(parser)](Properties const& props) mutable -> std::any {
      return std::any(parser(props));
    });
  }

  /** The type registered for \a name, EventType::Unknown if none */
  EventType find(std::string_view name) const;

  std::string_view get_name(EventType type) const;

  /** Set type and data of \a event from its name and properties */
  void resolve(FrameEvent& event) const;

private:
  EventType add_parser(std::string_view name, Parser parser);

private:
  struct Entry
  {
    std::string name;
    Parser parser;
  };

  /** Entry i is EventType i + 1 */
  std::vector<Entry> m_entries;
};

} // namespace wstsprite

#endif

/* EOF */
