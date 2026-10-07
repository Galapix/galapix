// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstsprite/sprite.hpp>

#include <wstsprite/sprite_manager.hpp>

namespace wstsprite {

namespace {

std::string const empty_action;

} // namespace

Sprite::Sprite(SpriteManager const& manager, SpriteId id) :
  m_manager(&manager),
  m_id(id),
  m_state()
{
}

SpriteData const*
Sprite::get_data() const
{
  return m_manager ? m_manager->find(m_id) : nullptr;
}

bool
Sprite::set_action(std::string_view name)
{
  SpriteData const* const data = get_data();
  return data && wstsprite::set_action(m_state, *data, name);
}

std::string const&
Sprite::get_action() const
{
  SpriteData const* const data = get_data();
  return data ? current_action(m_state, *data).name : empty_action;
}

void
Sprite::update(float delta, std::vector<EventRef>* events)
{
  if (SpriteData const* const data = get_data()) {
    advance(m_state, *data, delta, events);
  }
}

void
Sprite::draw(wstdisplay::Canvas& canvas, geom::fpoint const& pos) const
{
  draw(canvas, wstdisplay::DrawParams().set_pos(pos));
}

void
Sprite::draw(wstdisplay::Canvas& canvas, wstdisplay::DrawParams const& params) const
{
  if (SpriteData const* const data = get_data()) {
    wstsprite::draw(canvas, *data, m_state, params);
  }
}

} // namespace wstsprite

/* EOF */
