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

#ifndef HEADER_GALAPIX_RESOURCE_RESOURCE_ITEM_HPP
#define HEADER_GALAPIX_RESOURCE_RESOURCE_ITEM_HPP

#include <wstdisplay/fwd.hpp>

#include "galapix/workspace_item.hpp"
#include "resource/resource_locator.hpp"
#include "resource/resource_info.hpp"

class ResourceItem : public WorkspaceItem
{
public:
  ResourceItem(const ResourceLocator& locator);
  ~ResourceItem() override;

  void draw(wstdisplay::GraphicsContext& gc, const Rectf& cliprect, float zoom) override;
  void draw_mark(wstdisplay::GraphicsContext& gc) override;

  ResourceInfo get_resource_info() const;

  int get_original_width() const override { return 0; }
  int get_original_height() const override { return 0; }
  URL get_url() const override { return {}; }

private:
  void receive_resource_info(const ResourceInfo& resource_info);

private:
  ResourceLocator m_locator;
  ResourceInfo m_resource_info;

private:
  ResourceItem(const ResourceItem&);
  ResourceItem& operator=(const ResourceItem&);
};

#endif

/* EOF */
