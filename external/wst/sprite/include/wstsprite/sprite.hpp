// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTSPRITE_SPRITE_HPP
#define HEADER_WSTSPRITE_SPRITE_HPP

#include <string_view>
#include <vector>

#include <wstdisplay/canvas.hpp>

#include "anim_state.hpp"
#include "sprite_data.hpp"

namespace wstsprite {

class SpriteManager;

/** Convenience wrapper bundling a SpriteId with its AnimState, for code
    that doesn't keep them separately in components. The manager must
    outlive the sprite. An unloaded sprite behaves like an empty one. */
class Sprite final
{
public:
  Sprite() = default;
  Sprite(SpriteManager const& manager, SpriteId id);

  /** True if the sprite refers to loaded data */
  explicit operator bool() const { return get_data() != nullptr; }

  bool set_action(std::string_view name);
  std::string const& get_action() const;

  void update(float delta, std::vector<EventRef>* events = nullptr);
  void restart() { wstsprite::restart(m_state); }

  void draw(wstdisplay::Canvas& canvas, geom::fpoint const& pos) const;
  void draw(wstdisplay::Canvas& canvas, wstdisplay::DrawParams const& params) const;

  bool is_finished() const { return m_state.finished; }

  AnimState& get_state() { return m_state; }
  AnimState const& get_state() const { return m_state; }
  SpriteId get_id() const { return m_id; }

  /** The data, nullptr for an empty or unloaded sprite */
  SpriteData const* get_data() const;

private:
  SpriteManager const* m_manager = nullptr;
  SpriteId m_id = {};
  AnimState m_state = {};
};

} // namespace wstsprite

#endif

/* EOF */
