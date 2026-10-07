// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/lod/tile_types.hpp"

#include <cmath>
#include <utility>

namespace thumtoo::lod {

/// Finest live tile scale hosts request for PDF/DjVu/EPUB pages by default.
/// Thumtoo stores durable cells only for scale >= -2; finer is live-only.
inline constexpr int kDocumentLiveMinScale = -4;


/// Dimension at pyramid scale: successive integer floor-half (thumtoo/Galapix).
[[nodiscard]] inline int dim_at_tile_scale(int n, int scale) noexcept
{
  if (n <= 0) {
    return 0;
  }
  // Positive: successive floor-half (raster pyramid / PDF coarser).
  // Zero: layout / file-native size.
  // Negative: exact integer expand n * 2^{-scale} (PDF/DjVu/EPUB live denser
  // tiles — matches thumtoo pdf_page_size_at_scale). Raster images must keep
  // min_scale >= 0 so this branch is never used for photos.
  if (scale < 0) {
    // Cap expand to avoid overflow (host typically floors min_scale at -4..-6).
    int const exp = -scale;
    if (exp >= 20) {
      return 0;
    }
    long long const mul = 1LL << exp;
    long long const out = static_cast<long long>(n) * mul;
    if (out > 0x3fffffffLL) {
      return 0x3fffffff;
    }
    return static_cast<int>(out);
  }
  if (scale == 0) {
    return n;
  }
  for (int i = 0; i < scale; ++i) {
    n /= 2;
    if (n <= 0) {
      return 0;
    }
  }
  return n;
}

/// Number of tile columns/rows covering the **level** at scale s.
[[nodiscard]] inline int tiles_across(int content_dim, int scale) noexcept
{
  int const d = dim_at_tile_scale(content_dim, scale);
  if (d <= 0) {
    return 0;
  }
  return (d + kTileSize - 1) / kTileSize;
}

/// Smallest scale where the image fits in a single tile (or 0 if already).
[[nodiscard]] inline int max_scale_for_size(int width, int height,
                                            int scale_cap = 32) noexcept
{
  if (width <= 0 || height <= 0) {
    return 0;
  }
  int s = 0;
  while (s < scale_cap) {
    if (tiles_across(width, s) <= 1 && tiles_across(height, s) <= 1) {
      return s;
    }
    ++s;
  }
  return scale_cap;
}

/// Exclusive level-space pixel rect for tile (s,x,y) — same as thumtoo
/// `tile_cell_pixel_rect` on `dim_at_tile_scale(content,*)`.
[[nodiscard]] inline RectI tile_level_rect(int content_w, int content_h,
                                           TileKey const& key) noexcept
{
  if (content_w <= 0 || content_h <= 0) {
    return {};
  }
  int const scale = key.scale;
  int const level_w = dim_at_tile_scale(content_w, scale);
  int const level_h = dim_at_tile_scale(content_h, scale);
  int const nx = tiles_across(content_w, scale);
  int const ny = tiles_across(content_h, scale);
  if (nx <= 0 || ny <= 0 || key.x < 0 || key.y < 0 || key.x >= nx
      || key.y >= ny) {
    return {};
  }
  int const left = key.x * kTileSize;
  int const top = key.y * kTileSize;
  if (left >= level_w || top >= level_h) {
    return {};
  }
  int const tw = (kTileSize < (level_w - left)) ? kTileSize : (level_w - left);
  int const th = (kTileSize < (level_h - top)) ? kTileSize : (level_h - top);
  if (tw < 1 || th < 1) {
    return {};
  }
  return {left, top, tw, th};
}

/// Content-space rectangle for tile (s,x,y).
///
/// Level origin is exclusive (`level * 2^scale`). The last column/row is
/// stretched to the native content edge so floor-half residuals do not leave
/// a soft-underlay ring at coarse scales (looked like wrong tile scale when
/// navigating Image mode with lowres parents). Encode/source stay exclusive
/// ≤256; only the paint destination expands on the trailing edge.
[[nodiscard]] inline RectI tile_content_rect(int content_w, int content_h,
                                             TileKey const& key) noexcept
{
  RectI const lr = tile_level_rect(content_w, content_h, key);
  if (lr.empty()) {
    return {};
  }
  int const scale = key.scale;
  int const nx = tiles_across(content_w, scale);
  int const ny = tiles_across(content_h, scale);
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
  if (scale > 0) {
    // Coarser: level * 2^scale → content (edge stretch to native).
    int const factor = 1 << scale;
    x = lr.x * factor;
    y = lr.y * factor;
    w = lr.w * factor;
    h = lr.h * factor;
    if (key.x == nx - 1 && content_w > x) {
      w = content_w - x;
    }
    if (key.y == ny - 1 && content_h > y) {
      h = content_h - y;
    }
  } else if (scale < 0) {
    // Denser live tiles: level is content * 2^{-scale}; map back to layout.
    int const div = 1 << (-scale);
    x = lr.x / div;
    y = lr.y / div;
    // Ceil extent so edge cells still cover layout.
    w = (lr.w + div - 1) / div;
    h = (lr.h + div - 1) / div;
    if (w < 1) {
      w = 1;
    }
    if (h < 1) {
      h = 1;
    }
    if (x + w > content_w) {
      w = content_w - x;
    }
    if (y + h > content_h) {
      h = content_h - y;
    }
  } else {
    x = lr.x;
    y = lr.y;
    w = lr.w;
    h = lr.h;
    if (key.x == nx - 1 && content_w > x) {
      w = content_w - x;
    }
    if (key.y == ny - 1 && content_h > y) {
      h = content_h - y;
    }
  }
  if (w < 1 || h < 1) {
    return {};
  }
  return {x, y, w, h};
}

/// Parent cell at coarser scale (key.scale + delta), delta >= 1.
[[nodiscard]] inline TileKey parent_key(TileKey const& key, int delta) noexcept
{
  if (delta <= 0) {
    return key;
  }
  int const div = 1 << delta;
  return {key.scale + delta, key.x / div, key.y / div};
}

/// Integer power of two: 2^n for n in [0, 30].
[[nodiscard]] inline int pow2i(int n) noexcept
{
  if (n <= 0) {
    return 1;
  }
  if (n >= 30) {
    return 1 << 30;
  }
  return 1 << n;
}

/**
 * Target pyramid scale for a given device density.
 * device_per_content: screen pixels per content pixel (1 = 1:1).
 */
[[nodiscard]] inline int target_scale_for_density(double device_per_content,
                                                   int min_scale,
                                                   int max_scale) noexcept
{
  if (max_scale < min_scale) {
    return min_scale;
  }
  if (!(device_per_content > 0.0) || !std::isfinite(device_per_content)) {
    return max_scale;
  }
  double const ideal = -std::log2(device_per_content);
  int s = static_cast<int>(std::floor(ideal + 1e-9));
  if (s < min_scale) {
    s = min_scale;
  }
  if (s > max_scale) {
    s = max_scale;
  }
  return s;
}

/// UV rect in parent bitmap covering the fine tile's content, for CoarserTile.
[[nodiscard]] inline RectF parent_uv_for_child(TileKey const& fine,
                                               TileKey const& parent,
                                               int parent_pixel_w,
                                               int parent_pixel_h,
                                               int content_w,
                                               int content_h) noexcept
{
  if (parent_pixel_w <= 0 || parent_pixel_h <= 0) {
    return {};
  }
  if (parent.scale <= fine.scale) {
    return {0, 0, static_cast<double>(parent_pixel_w),
            static_cast<double>(parent_pixel_h)};
  }
  RectI const fine_cr = tile_content_rect(content_w, content_h, fine);
  RectI const parent_cr = tile_content_rect(content_w, content_h, parent);
  if (fine_cr.empty() || parent_cr.empty() || parent_cr.w <= 0
      || parent_cr.h <= 0) {
    return {0, 0, static_cast<double>(parent_pixel_w),
            static_cast<double>(parent_pixel_h)};
  }
  // Parent bitmap is exclusive level pixels. parent_cr may be edge-stretched
  // to the native AABB; map UV proportionally onto the exclusive payload.
  int const map_w = parent_pixel_w;
  int const map_h = parent_pixel_h;
  double const u0 =
      (static_cast<double>(fine_cr.x - parent_cr.x)
       / static_cast<double>(parent_cr.w))
      * static_cast<double>(map_w);
  double const v0 =
      (static_cast<double>(fine_cr.y - parent_cr.y)
       / static_cast<double>(parent_cr.h))
      * static_cast<double>(map_h);
  double const uw =
      (static_cast<double>(fine_cr.w) / static_cast<double>(parent_cr.w))
      * static_cast<double>(map_w);
  double const vh =
      (static_cast<double>(fine_cr.h) / static_cast<double>(parent_cr.h))
      * static_cast<double>(map_h);
  return {u0, v0, uw, vh};
}

}  // namespace thumtoo::lod

