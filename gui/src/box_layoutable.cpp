// SPDX-FileCopyrightText: 2018 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/box_layoutable.hpp>

namespace wstgui {

BoxLayoutable::BoxLayoutable(Orientation orientation) :
  m_orientation(orientation),
  m_items()
{
}

void
BoxLayoutable::pack(Component* component, bool expand, bool fill, float margin)
{
  m_items.emplace_back(Item{std::make_unique<LayoutableComponent>(component), expand, fill, margin});
}

void
BoxLayoutable::pack(std::unique_ptr<ILayoutable> layoutable, bool expand, bool fill, float margin)
{
  m_items.emplace_back(Item{std::move(layoutable), expand, fill, margin});
}

void
BoxLayoutable::pack_spacer(float length)
{
  m_items.emplace_back(Item{std::make_unique<SpacerLayoutable>(geom::fsize(length, length)), false, false, 0.0f});
}

void
BoxLayoutable::pack_stretcher()
{
  m_items.emplace_back(Item{std::make_unique<ILayoutable>(), true, true, 0.0f});
}

void
BoxLayoutable::set_geometry(geom::frect const& rect)
{
  auto get_length = [&](geom::frect const& size_hint) {
    if (m_orientation == Orientation::Horizontal) {
      return size_hint.width();
    } else {
      return size_hint.height();
    }
  };

  auto set_pos_length = [&](auto& item, float pos, float length) {
    if (m_orientation == Orientation::Horizontal) {
      item.layoutable->set_geometry(geom::frect(pos,
                                                rect.top(),
                                                pos + length,
                                                rect.bottom()));
    } else {
      item.layoutable->set_geometry(geom::frect(rect.left(),
                                                pos,
                                                rect.right(),
                                                pos + length));
    }
  };

  float fixed_length = 0;
  int num_expanding = 0;
  for (auto const& item : m_items)
  {
    if (auto size_hint =  item.layoutable->get_size_hint()) {
      fixed_length += get_length(*size_hint);
    } else {
      num_expanding += 1;
    }
  }

  float const expanding_length = (num_expanding == 0) ? 0.0f : ((get_length(rect) - fixed_length) / static_cast<float>(num_expanding));

  float pos = (m_orientation == Orientation::Horizontal) ? rect.left() : rect.top();
  for (Item& item : m_items)
  {
    float new_length;
    if (std::optional<geom::frect> size_hint = item.layoutable->get_size_hint()) {
      new_length = get_length(*size_hint);
    } else {
      new_length = expanding_length;
    }
    set_pos_length(item, pos, new_length);

    pos += new_length;
  }
}

} // namespace wstgui

/* EOF */
