// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#include <wstdisplay/rect_packer.hpp>

#include <algorithm>
#include <limits>

namespace wstdisplay {

RectPacker::RectPacker(geom::isize const& size) :
  m_size(size),
  m_skyline(),
  m_used_area(0)
{
  clear();
}

void
RectPacker::clear()
{
  m_skyline.clear();
  m_skyline.push_back(Node{0, 0, m_size.width()});
  m_used_area = 0;
}

int
RectPacker::fit(size_t index, int width, int height) const
{
  int const x = m_skyline[index].x;
  if (x + width > m_size.width()) {
    return -1;
  }

  int y = m_skyline[index].y;
  int remaining = width;
  for (size_t i = index; remaining > 0; ++i) {
    if (i >= m_skyline.size()) {
      return -1;
    }
    y = std::max(y, m_skyline[i].y);
    if (y + height > m_size.height()) {
      return -1;
    }
    remaining -= m_skyline[i].width;
  }
  return y;
}

std::optional<geom::irect>
RectPacker::allocate(geom::isize const& size)
{
  if (size.width() <= 0 || size.height() <= 0 ||
      size.width() > m_size.width() || size.height() > m_size.height()) {
    return std::nullopt;
  }

  // pick the position with the lowest top edge, ties broken by the
  // narrowest node to reduce fragmentation
  int best_y = std::numeric_limits<int>::max();
  int best_width = std::numeric_limits<int>::max();
  size_t best_index = m_skyline.size();

  for (size_t i = 0; i < m_skyline.size(); ++i) {
    int const y = fit(i, size.width(), size.height());
    if (y >= 0) {
      int const bottom = y + size.height();
      if (bottom < best_y || (bottom == best_y && m_skyline[i].width < best_width)) {
        best_y = bottom;
        best_width = m_skyline[i].width;
        best_index = i;
      }
    }
  }

  if (best_index == m_skyline.size()) {
    return std::nullopt;
  }

  geom::irect const rect(geom::ipoint(m_skyline[best_index].x, best_y - size.height()), size);

  // insert the new skyline segment and shrink the ones it covers
  m_skyline.insert(m_skyline.begin() + static_cast<std::ptrdiff_t>(best_index),
                   Node{rect.left(), rect.bottom(), rect.width()});

  for (size_t i = best_index + 1; i < m_skyline.size();) {
    Node& prev = m_skyline[i - 1];
    Node& node = m_skyline[i];
    int const prev_right = prev.x + prev.width;
    if (node.x < prev_right) {
      int const shrink = prev_right - node.x;
      node.x += shrink;
      node.width -= shrink;
      if (node.width <= 0) {
        m_skyline.erase(m_skyline.begin() + static_cast<std::ptrdiff_t>(i));
        continue;
      }
    }
    break;
  }

  // merge neighbors on the same height
  for (size_t i = 0; i + 1 < m_skyline.size();) {
    if (m_skyline[i].y == m_skyline[i + 1].y) {
      m_skyline[i].width += m_skyline[i + 1].width;
      m_skyline.erase(m_skyline.begin() + static_cast<std::ptrdiff_t>(i + 1));
    } else {
      ++i;
    }
  }

  m_used_area += static_cast<long>(size.width()) * size.height();
  return rect;
}

float
RectPacker::get_occupancy() const
{
  return static_cast<float>(m_used_area) /
    static_cast<float>(static_cast<long>(m_size.width()) * m_size.height());
}

} // namespace wstdisplay

/* EOF */
