/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmail.com>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "galapix/layouter/tight_layouter.hpp"

#include <iostream>

#include "galapix/image.hpp"
#include "galapix/image_collection.hpp"

TightLayouter::TightLayouter(float w, float h) :
  m_aspect_w(w),
  m_aspect_h(h)
{
}

void
TightLayouter::layout(const ImageCollection& images)
{
  float spacing = 24.0f;

  float width = 0;
  // calculate the total width
  for(const auto& image: images)
  {
    const float scale = (1000.0f + spacing) / static_cast<float>(image->get_original_height());
    width += static_cast<float>(image->get_original_width()) * scale;
  }

  width /= Math::sqrt(width / ((m_aspect_w / m_aspect_h) * (1000.0f + spacing)));

  Vector2f pos(0.0f, 0.0f);
  Vector2f last_pos(0.0f, 0.0f);
  bool go_right = true;

  auto relayout_row = [width, spacing](std::vector<WorkspaceItemPtr>& row) {
    float row_width = spacing * static_cast<float>(row.size()-1);
    for(auto& image: row) { row_width += image->get_scaled_width(); }

    //std::cout << width << " " << row_width << std::endl;
    Vector2f offset((width - row_width)/2.0f, 0.0f);

    // FIXME: doesn't work because of target pos
    // center the row
    //for(auto& image: row) {
    //image->set_pos(image->get_target_pos() - offset);
    //}

    row.clear();
  };

  std::vector<WorkspaceItemPtr> row;
  for(const auto& image: images)
  {
    row.push_back(image);

    const float scale = 1000.0f / static_cast<float>(image->get_original_height());
    image->set_scale(scale);

    auto set_pos = [&](const Vector2f& p){
      last_pos = p;
      image->set_pos(p + Vector2f(static_cast<float>(image->get_original_width()),
                                         static_cast<float>(image->get_original_height())) * scale / 2.0f);
    };

    if (go_right)
    {
      // going right
      if (pos.x + (static_cast<float>(image->get_original_width())*scale) > width)
      {
        pos.x = last_pos.x;
        pos.y += 1000.0f + spacing;

        go_right = false;

        set_pos(pos);
        relayout_row(row);
      }
      else
      {
        set_pos(pos);
        pos.x += static_cast<float>(image->get_original_width()) * scale + spacing;
      }
    }
    else
    {
      // going left
      if (pos.x - (static_cast<float>(image->get_original_width()) * scale) < 0)
      {
        pos.y += 1000.0f + spacing;
        go_right = true;

        set_pos(pos);
        pos.x += static_cast<float>(image->get_original_width()) * scale + spacing;

        relayout_row(row);
      }
      else
      {
        pos.x -= static_cast<float>(image->get_original_width()) * scale + spacing;
        set_pos(pos);
      }
    }
  }
  relayout_row(row);
}

void
TightLayouter::layout_zigzag(const ImageCollection& images)
{
  float spacing = 24.0f;

  float width = 0;
  // calculate the total width
  for(const auto& image: images)
  {
    const float scale = (1000.0f + spacing) / static_cast<float>(image->get_original_height());
    width += static_cast<float>(image->get_original_width()) * scale;
  }

  width /= Math::sqrt(width / ((m_aspect_w / m_aspect_h) * (1000.0f + spacing)));

  Vector2f pos(0.0f, 0.0f);
  Vector2f last_pos(0.0f, 0.0f);
  bool go_right = true;

  for(const auto& image: images)
  {
    const float scale = 1000.0f / static_cast<float>(image->get_original_height());
    image->set_scale(scale);

    auto set_pos = [&](const Vector2f& p){
        last_pos = p;
        image->set_pos(p + Vector2f(static_cast<float>(image->get_original_width()),
                                           static_cast<float>(image->get_original_height())) * scale / 2.0f);
      };

    if (go_right)
    {
      // going right
      if (pos.x + (static_cast<float>(image->get_original_width())*scale) > width)
      {
        pos.x = last_pos.x;
        pos.y += 1000.0f + spacing;

        go_right = false;

        set_pos(pos);
      }
      else
      {
        set_pos(pos);
        pos.x += static_cast<float>(image->get_original_width()) * scale + spacing;
      }
    }
    else
    {
      // going left
      if (pos.x - (static_cast<float>(image->get_original_width()) * scale) < 0)
      {
        pos.y += 1000.0f + spacing;
        go_right = true;

        set_pos(pos);
        pos.x += static_cast<float>(image->get_original_width()) * scale + spacing;
      }
      else
      {
        pos.x -= static_cast<float>(image->get_original_width()) * scale + spacing;
        set_pos(pos);
      }
    }
  }
}

/* EOF */
