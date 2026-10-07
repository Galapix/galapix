// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: LGPL-3.0-or-later

#ifndef HEADER_WSTDISPLAY_RECT_PACKER_HPP
#define HEADER_WSTDISPLAY_RECT_PACKER_HPP

#include <optional>
#include <vector>

#include <geom/rect.hpp>
#include <geom/size.hpp>

namespace wstdisplay {

/** Packs rectangles into an area of fixed size using the skyline
    bottom-left heuristic. Pure CPU, used for atlas pages. */
class RectPacker final
{
public:
  explicit RectPacker(geom::isize const& size);

  /** Find room for a rectangle of \a size, std::nullopt if full */
  std::optional<geom::irect> allocate(geom::isize const& size);

  void clear();

  geom::isize get_size() const { return m_size; }

  /** Fraction of the area that has been allocated */
  float get_occupancy() const;

private:
  struct Node
  {
    int x;
    int y;
    int width;
  };

  /** y position a rect of \a width would get at node \a index, or -1 */
  int fit(size_t index, int width, int height) const;

private:
  geom::isize m_size;
  std::vector<Node> m_skyline;
  long m_used_area;
};

} // namespace wstdisplay

#endif

/* EOF */
