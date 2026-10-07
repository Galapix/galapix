// SPDX-FileCopyrightText: 2005 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/label.hpp>

#include <surf/color.hpp>

#include <wstgui/style.hpp>

namespace wstgui {

Label::Label(const std::string& label_, Component* parent_)
  : Component(parent_),
    label(label_)
{
}

Label::~Label()
{
}

void
Label::draw(wstdisplay::Canvas& canvas)
{
  //canvas.fill_rect(rect, surf::Color(0.0f, 0.0f, 0.0f, 0.5f));
  //canvas.draw_rect(rect, surf::Color(1.0f, 1.0f, 1.0f, 0.5f));

  get_style().get_small_font()->draw(
    canvas,
    glm::vec2(m_geometry.left() + 5/*+ rect.width()/2*/,
              m_geometry.top() + m_geometry.height()/2 + 3),
    label,
    surf::Color(1.0f, 1.0f, 1.0f, 1.0f));
}

void
Label::update(float /*delta*/, const Controller& /*controller*/)
{
  set_active(false);
}

} // namespace wstgui

/* EOF */
