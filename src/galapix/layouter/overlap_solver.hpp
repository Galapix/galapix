// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_GALAPIX_LAYOUTER_OVERLAP_SOLVER_HPP
#define HEADER_GALAPIX_GALAPIX_LAYOUTER_OVERLAP_SOLVER_HPP

#include "galapix/image_collection.hpp"

namespace galapix {

/** Push images apart until axis-aligned bounds no longer overlap.
 *
 *  Uses a spatial hash + iterative minimum-translation separation so
 *  cost stays near O(n + overlaps) per pass instead of O(n²) full
 *  pairwise scans. Positions stay close to the current layout (tight
 *  packing preserved as much as pure separation allows).
 *
 *  @param gap Extra clearance between resolved bounds (world units).
 *  @return Number of separation iterations performed.
 */
int solve_image_overlaps(ImageCollection& images, float gap = 16.0f);

} // namespace galapix

#endif

/* EOF */
