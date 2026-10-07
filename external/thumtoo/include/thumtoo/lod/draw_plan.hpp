// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/lod/tile_types.hpp"

#include <functional>
#include <vector>

namespace thumtoo::lod {

/**
 * Lookup into the path's TileLoader cells (nullptr = Missing). Only Ready
 * cells are drawn. Pure relative to the lookup — no side effects.
 */
using TileLookup = std::function<TileCell const*(TileKey const&)>;

struct BuildDrawPlanInput {
  int content_w = 0;
  int content_h = 0;
  int target_scale = 0;
  int max_scale = 0;
  std::vector<TileKey> visible_keys;
  TileLookup lookup;  ///< Required; returns nullptr if Missing
  bool has_lqip = false;
};

/**
 * For each visible key: exact tile → coarser parent → LQIP flag → empty.
 * Does not enqueue requests. UV is in tile pixel space of the chosen source.
 */
[[nodiscard]] DrawPlan build_draw_plan(BuildDrawPlanInput const& in);

}  // namespace thumtoo::lod

