// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTSPRITE_SPRITE_DATA_HPP
#define HEADER_WSTSPRITE_SPRITE_DATA_HPP

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <geom/point.hpp>
#include <geom/rect.hpp>
#include <wstdisplay/fwd.hpp>
#include <wstdisplay/handle.hpp>
#include <wstdisplay/surface.hpp>

#include "frame_event.hpp"
#include "properties.hpp"

namespace prio {
class ReaderMapping;
} // namespace prio

namespace wstsprite {

struct Frame
{
  wstdisplay::Surface surface = {};

  /** How long the frame is shown, in seconds */
  float duration = 0.1f;
};

struct Action
{
  std::string name = {};
  std::vector<Frame> frames = {};

  /** The point of the frame, in frame pixels, that is placed at the
      draw position and that rotation and scaling are relative to */
  geom::fpoint origin = {};

  float scale = 1.0f;

  /** Draw the frames mirrored, used for "mirror-action" */
  bool hflip = false;

  /** Number of times the animation plays, -1 loops forever */
  int loops = -1;

  /** Frame the animation restarts at when looping */
  int loop_frame = 0;

  /** Collision box in frame pixels, SuperTux "hitbox" */
  std::optional<geom::frect> hitbox = {};

  std::vector<FrameEvent> events = {};

  /** Keys not interpreted by the library */
  Properties properties = {};

  float get_total_duration() const;
};

/** The animations of a sprite, loaded from one of the .sprite formats
    of SuperTux, Windstille or Pingus, detected by the root element:

    (supertux-sprite (actions (action (name ..) (fps ..) (images ..) ...) ...))
    (sprite (actions (action (name ..) (speed ..) (images ..) ...) ...))
    (pingus-sprite (image ..) (array ..) (speed ..) (origin ..) ...)

    All formats accept an (events ...) list per action, see FrameEvent,
    which are converted to typed data by an optional EventRegistry.

    SpriteData is a plain value, SpriteManager owns the loaded ones and
    hands out SpriteId handles to store in components. */
class SpriteData final
{
public:
  /** Load \a filename, images are loaded through \a surfaces,
      events are parsed with \a events if given */
  static SpriteData from_file(std::filesystem::path const& filename, wstdisplay::SurfaceManager& surfaces,
                              EventRegistry const* events = nullptr);

  /** Load the sprite element \a name with \a mapping, images are
      relative to \a directory. Use this for sprites embedded in other
      files, e.g. Pingus animation sets. */
  static SpriteData from_mapping(std::string_view name, prio::ReaderMapping const& mapping,
                                 std::filesystem::path const& directory,
                                 wstdisplay::SurfaceManager& surfaces,
                                 EventRegistry const* events = nullptr);

  /** A single image as a sprite with one action "default" */
  static SpriteData from_surface(wstdisplay::Surface const& surface);

public:
  SpriteData();

  std::vector<Action> const& get_actions() const { return m_actions; }

  /** Index of the action \a name, -1 if there is none */
  int find_action(std::string_view name) const;

  Action const& get_action(int index) const { return m_actions[static_cast<size_t>(index)]; }

  FrameEvent const& get_event(EventRef ref) const
  {
    return get_action(ref.action).events[static_cast<size_t>(ref.event)];
  }

  void add_action(Action action) { m_actions.push_back(std::move(action)); }

private:
  std::vector<Action> m_actions;
};

/** A loaded sprite in a SpriteManager, a trivially copyable value for
    components, see wstdisplay::Handle */
using SpriteId = wstdisplay::Handle<SpriteData>;

} // namespace wstsprite

#endif

/* EOF */
