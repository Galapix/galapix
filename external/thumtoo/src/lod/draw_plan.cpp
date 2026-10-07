// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/lod/draw_plan.hpp"

#include "thumtoo/lod/lod_math.hpp"

namespace thumtoo::lod {

DrawPlan build_draw_plan(BuildDrawPlanInput const& in)
{
  DrawPlan plan;
  plan.target_scale = in.target_scale;
  if (!in.lookup || in.content_w <= 0 || in.content_h <= 0) {
    return plan;
  }

  plan.commands.reserve(in.visible_keys.size());

  for (TileKey const& key : in.visible_keys) {
    DrawCommand cmd;
    RectI const cr = tile_content_rect(in.content_w, in.content_h, key);
    if (cr.empty()) {
      continue;
    }
    cmd.dst_content = {static_cast<double>(cr.x), static_cast<double>(cr.y),
                       static_cast<double>(cr.w), static_cast<double>(cr.h)};

    // 1. Exact
    //   level rect L = exclusive cell on dim_at_tile_scale(content, s)
    //   dest = L * 2^s   (integer, from tile_content_rect)
    //   src  = [0,0]×L   (cap width/height to bitmap; legacy 257 → first 256)
    if (TileCell const* exact = in.lookup(key); exact && exact->ready()) {
      cmd.src_key = key;
      // Full bitmap → content-space cell. DCT/floor-half size drift is
      // stretched here; do not crop to tile_level_rect or the cell shows
      // a partial sample / falls through to LQIP underlay.
      cmd.src_uv = {0, 0, static_cast<double>(exact->bitmap.width),
                    static_cast<double>(exact->bitmap.height)};
      cmd.dst_content = {static_cast<double>(cr.x), static_cast<double>(cr.y),
                         static_cast<double>(cr.w), static_cast<double>(cr.h)};
      cmd.kind = DrawKind::ExactTile;
      plan.any_tile = true;
      plan.commands.push_back(cmd);
      continue;
    }

    // 2. Finest available coarser parent
    bool found_parent = false;
    int const max_delta = std::max(0, in.max_scale - key.scale);
    for (int delta = 1; delta <= max_delta; ++delta) {
      TileKey const pk = parent_key(key, delta);
      TileCell const* parent = in.lookup(pk);
      if (!parent || !parent->ready()) {
        continue;
      }
      cmd.src_key = pk;
      cmd.src_uv = parent_uv_for_child(key, pk, parent->bitmap.width,
                                       parent->bitmap.height, in.content_w,
                                       in.content_h);
      cmd.kind = DrawKind::CoarserTile;
      plan.any_tile = true;
      plan.commands.push_back(cmd);
      found_parent = true;
      break;
    }
    if (found_parent) {
      continue;
    }

    // 3. Hole: soft underlay shows through unless LQIP is available.
    if (in.has_lqip) {
      cmd.kind = DrawKind::Underlay;
      cmd.use_lqip = true;
      plan.commands.push_back(cmd);
    }
  }

  return plan;
}

}  // namespace thumtoo::lod
