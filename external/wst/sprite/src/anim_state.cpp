// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstsprite/anim_state.hpp>

#include <algorithm>

#include <logmich/log.hpp>
#include <wstdisplay/surface.hpp>

namespace wstsprite {

namespace {

void append_events(AnimState const& state, Action const& action, std::vector<EventRef>* events)
{
  if (!events) {
    return;
  }
  for (size_t i = 0; i < action.events.size(); ++i) {
    if (action.events[i].frame == state.frame) {
      events->push_back(EventRef{state.action, static_cast<int>(i)});
    }
  }
}

} // namespace

void
restart(AnimState& state)
{
  state.frame = 0;
  state.frame_time = 0.0f;
  state.loops_done = 0;
  state.finished = false;
}

void
set_action(AnimState& state, int action)
{
  state.action = action;
  restart(state);
}

bool
set_action(AnimState& state, SpriteData const& data, std::string_view name)
{
  int const idx = data.find_action(name);
  if (idx < 0) {
    log_debug("set_action: unknown action '{}'", name);
    return false;
  }
  set_action(state, idx);
  return true;
}

bool
set_action_continued(AnimState& state, SpriteData const& data, std::string_view name)
{
  int const idx = data.find_action(name);
  if (idx < 0) {
    log_debug("set_action_continued: unknown action '{}'", name);
    return false;
  }
  state.action = idx;
  int const frames = static_cast<int>(data.get_action(idx).frames.size());
  state.frame = std::clamp(state.frame, 0, std::max(0, frames - 1));
  return true;
}

Action const&
current_action(AnimState const& state, SpriteData const& data)
{
  return data.get_action(state.action);
}

Frame const&
current_frame(AnimState const& state, SpriteData const& data)
{
  return current_action(state, data).frames[static_cast<size_t>(state.frame)];
}

void
fire_current_events(AnimState const& state, SpriteData const& data,
                    std::vector<EventRef>& events)
{
  append_events(state, current_action(state, data), &events);
}

void
advance(AnimState& state, SpriteData const& data, float delta,
        std::vector<EventRef>* events)
{
  Action const& action = current_action(state, data);
  int const frame_count = static_cast<int>(action.frames.size());
  if (state.finished || state.speed <= 0.0f || frame_count == 0) {
    return;
  }

  state.frame_time += delta * state.speed;

  while (true)
  {
    float const duration = action.frames[static_cast<size_t>(state.frame)].duration;
    if (duration <= 0.0f || state.frame_time < duration) {
      break;
    }

    int next = state.frame + 1;
    if (next >= frame_count) {
      if (action.loops < 0 || state.loops_done + 1 < action.loops) {
        state.loops_done += 1;
        next = std::clamp(action.loop_frame, 0, frame_count - 1);
      } else {
        state.loops_done += 1;
        state.finished = true;
        state.frame_time = 0.0f;
        break;
      }
    }

    state.frame_time -= duration;
    state.frame = next;
    append_events(state, action, events);
  }
}

void
draw(wstdisplay::Canvas& canvas, SpriteData const& data, AnimState const& state,
     wstdisplay::DrawParams const& params)
{
  Action const& action = current_action(state, data);
  if (action.frames.empty()) {
    return;
  }

  wstdisplay::DrawParams p = params;
  p.anchor = geom::origin::TOP_LEFT;
  p.offset = geom::foffset(-action.origin.x(), -action.origin.y()) + params.offset;
  p.scale *= action.scale;
  p.hflip = (params.hflip != action.hflip);
  canvas.draw(action.frames[static_cast<size_t>(state.frame)].surface, p);
}

} // namespace wstsprite

/* EOF */
