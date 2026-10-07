// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTSPRITE_ANIM_STATE_HPP
#define HEADER_WSTSPRITE_ANIM_STATE_HPP

#include <cstdint>
#include <string_view>
#include <type_traits>
#include <vector>

#include <wstdisplay/canvas.hpp>
#include <wstdisplay/draw_params.hpp>

#include "sprite_data.hpp"

namespace wstsprite {

/** The playback state of an animation. A small trivially copyable
    struct, meant to be stored directly in an ECS component next to a
    SpriteId. Time only advances in advance(), never when drawing,
    so game logic stays deterministic for a fixed time step. */
struct AnimState
{
  int action = 0;
  int frame = 0;

  /** Time spent in the current frame, in seconds */
  float frame_time = 0.0f;

  /** Number of completed loops */
  int loops_done = 0;

  /** Playback speed factor, 0 pauses, negative values are not supported */
  float speed = 1.0f;

  /** A non-looping animation reached its end */
  bool finished = false;
};

static_assert(std::is_trivially_copyable_v<AnimState>);

/** Switch to \a action and restart it, unknown names are ignored,
    logged at debug level and return false */
bool set_action(AnimState& state, SpriteData const& data, std::string_view name);
void set_action(AnimState& state, int action);

/** Switch to \a action keeping the current frame and time, e.g.
    walk-left to walk-right without restarting */
bool set_action_continued(AnimState& state, SpriteData const& data, std::string_view name);

void restart(AnimState& state);

/** Advance the animation by \a delta seconds. Events of frames that
    are entered are appended to \a events, which can be nullptr. The
    events of the first frame don't fire on restart() or set_action(),
    use fire_current_events() for those. */
void advance(AnimState& state, SpriteData const& data, float delta,
             std::vector<EventRef>* events = nullptr);

/** Events of the current frame, call after set_action()/restart() if
    frame 0 events should fire */
void fire_current_events(AnimState const& state, SpriteData const& data,
                         std::vector<EventRef>& events);

Action const& current_action(AnimState const& state, SpriteData const& data);
Frame const& current_frame(AnimState const& state, SpriteData const& data);

/** Draw the current frame, \a params.pos is where the action origin
    goes. The action scale and mirroring are applied on top of
    \a params. */
void draw(wstdisplay::Canvas& canvas, SpriteData const& data, AnimState const& state,
          wstdisplay::DrawParams const& params);

} // namespace wstsprite

#endif

/* EOF */
