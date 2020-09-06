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

#ifndef HEADER_GALAPIX_GALAPIX_LAYOUTER_TIGHT_LAYOUTER_HPP
#define HEADER_GALAPIX_GALAPIX_LAYOUTER_TIGHT_LAYOUTER_HPP

#include "galapix/layouter/layouter.hpp"

#include "math/vector2f.hpp"

class Image;

class TightLayouter : public Layouter
{
public:
  TightLayouter(float w, float h);

  void layout_zigzag(const ImageCollection& images) const;
  void layout(const ImageCollection& images) override;

private:
  float m_aspect_w;
  float m_aspect_h;

private:
  TightLayouter(const TightLayouter&);
  TightLayouter& operator=(const TightLayouter&);
};

#endif

/* EOF */
