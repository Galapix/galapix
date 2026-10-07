// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/lod/lod_planner.hpp"

#include <algorithm>
#include <cmath>

namespace thumtoo::lod {

PlannerOutput plan_visible_tiles(PlannerInput const& in)
{
  PlannerOutput out;
  if (in.content_w <= 0 || in.content_h <= 0) {
    return out;
  }

  int max_s = in.max_scale;
  if (max_s < in.min_scale) {
    max_s = max_scale_for_size(in.content_w, in.content_h);
  }
  if (max_s < in.min_scale) {
    max_s = in.min_scale;
  }

  out.target_scale = target_scale_for_density(in.viewport.device_per_content,
                                             in.min_scale, max_s);

  RectF vis = in.viewport.content_rect;
  if (in.margin_content > 0.0) {
    vis.x -= in.margin_content;
    vis.y -= in.margin_content;
    vis.w += 2.0 * in.margin_content;
    vis.h += 2.0 * in.margin_content;
  }

  // Clamp query region to content bounds (still allow slight margin outside).
  RectF const content_bounds{0, 0, static_cast<double>(in.content_w),
                             static_cast<double>(in.content_h)};
  RectF const query = vis.intersection(content_bounds);
  if (query.empty()) {
    return out;
  }

  int const s = out.target_scale;
  int const tw = tiles_across(in.content_w, s);
  int const th = tiles_across(in.content_h, s);
  if (tw <= 0 || th <= 0) {
    return out;
  }

  // Content-space stride of one tile cell at scale s.
  // Positive s: kTileSize * 2^s. Scale 0: kTileSize.
  // Negative s: kTileSize / 2^{-s}. Using kTileSize for s < 0 skipped half
  // the keys at −1 (wrong clip / incomplete coverage).
  int step = kTileSize;
  if (s > 0) {
    step = kTileSize * pow2i(s);
  } else if (s < 0) {
    int const div = pow2i(-s);
    step = (div > 0) ? (kTileSize / div) : kTileSize;
    if (step < 1) {
      step = 1;
    }
  }

  // Inclusive tile index range covering the query rect.
  int x0 = static_cast<int>(std::floor(query.x / static_cast<double>(step)));
  int y0 = static_cast<int>(std::floor(query.y / static_cast<double>(step)));
  int x1 = static_cast<int>(
      std::floor((query.right() - 1e-9) / static_cast<double>(step)));
  int y1 = static_cast<int>(
      std::floor((query.bottom() - 1e-9) / static_cast<double>(step)));

  x0 = std::max(0, x0);
  y0 = std::max(0, y0);
  x1 = std::min(tw - 1, x1);
  y1 = std::min(th - 1, y1);

  out.visible_keys.reserve(static_cast<size_t>((x1 - x0 + 1) * (y1 - y0 + 1)));
  for (int y = y0; y <= y1; ++y) {
    for (int x = x0; x <= x1; ++x) {
      TileKey const key{s, x, y};
      RectI const cr = tile_content_rect(in.content_w, in.content_h, key);
      if (cr.empty()) {
        continue;
      }
      RectF const fr{static_cast<double>(cr.x), static_cast<double>(cr.y),
                     static_cast<double>(cr.w), static_cast<double>(cr.h)};
      if (fr.intersects(query)) {
        out.visible_keys.push_back(key);
      }
    }
  }

  return out;
}

}  // namespace thumtoo::lod
