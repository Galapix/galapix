// SPDX-FileCopyrightText: 2021 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include <wstgui/anchor_layoutable.hpp>

#include <logmich/log.hpp>
#include <geom/io.hpp>

namespace wstgui {

AnchorLayoutable::AnchorLayoutable() :
  m_items()
{
}

void
AnchorLayoutable::set_geometry(geom::frect const& geometry)
{
  for (auto const& item : m_items)
  {
    geom::frect rect = geom::anchored_rect(geom::fpoint(item.x(geometry.width()),
                                                  item.y(geometry.height())),
                                           geom::fsize(item.width(geometry.width()),
                                                       item.height(geometry.height())),
                                           item.origin);
    item.layoutable->set_geometry(rect);
  }
}

void
AnchorLayoutable::place(std::unique_ptr<ILayoutable> layoutable,
                        rfloat x, rfloat y, rfloat w, rfloat h, geom::origin origin)
{
  m_items.emplace_back(Item{std::move(layoutable), x, y, w, h, origin});
}

void
AnchorLayoutable::place(Component* component,
                        rfloat x, rfloat y, rfloat w, rfloat h, geom::origin origin)
{
  place(std::make_unique<LayoutableComponent>(component), x, y, w, h, origin);
}

void
AnchorLayoutable::place(std::unique_ptr<ILayoutable> layoutable, geom::frect const& rect)
{
  place(std::move(layoutable),
        rfloat::absolute(rect.x()), rfloat::absolute(rect.y()),
        rfloat::absolute(rect.size().width()), rfloat::absolute(rect.size().height()),
        geom::origin::TOP_LEFT);
}

void
AnchorLayoutable::place(Component* component, geom::frect const& rect)
{
  place(std::make_unique<LayoutableComponent>(component), rect);
}

} // namespace wstgui

/* EOF */
