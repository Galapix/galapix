// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "resource_item.hpp"

ResourceItem::ResourceItem(ResourceLocator const& locator) :
  m_locator(locator),
  m_resource_info()
{
}

ResourceItem::~ResourceItem()
{
}
#if 0
void
ResourceItem::on_startup()
{
  // m_resource_info = ResourceManager::current()->request_resource_info(m_locator);
}

void
ResourceItem::on_shutdown()
{
  // cancel jobs
}
#endif

void
ResourceItem::draw(wstdisplay::GraphicsContext& gc, Rectf const& cliprect, float zoom)
{
}

void
ResourceItem::draw_mark(wstdisplay::GraphicsContext& gc)
{
}

void
ResourceItem::receive_resource_info(ResourceInfo const& resource_info)
{
  // store resource_info
  m_resource_info = resource_info;

  // create child items
  //for(const auto& locator : m_resource_info.get_children())
  {
    //  add_child(std::make_shared<ResourceItem>(locator));
  }

  //m_renderer = Renderer::from_resource_info(m_resource_info);
}

/* EOF */
