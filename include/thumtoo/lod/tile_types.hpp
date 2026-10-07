// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace thumtoo::lod {

/// Matches thumtoo::kTileSize / Galapix tile grid. Exclusive ≤256 cells;
/// no edge overlap strip (that experiment is gone on both sides).
inline constexpr int kTileSize = 256;

/// Grid cell identity at a pyramid scale.
struct TileKey {
  int scale = 0;  ///< 0 = full resolution; +1 halves linear size
  int x = 0;
  int y = 0;

  bool operator==(TileKey const& o) const noexcept
  {
    return scale == o.scale && x == o.x && y == o.y;
  }
  bool operator!=(TileKey const& o) const noexcept { return !(*this == o); }
  bool operator<(TileKey const& o) const noexcept
  {
    if (scale != o.scale) {
      return scale < o.scale;
    }
    if (x != o.x) {
      return x < o.x;
    }
    return y < o.y;
  }
};

/// Axis-aligned rectangle in continuous space (content or UV).
struct RectF {
  double x = 0;
  double y = 0;
  double w = 0;
  double h = 0;

  double right() const noexcept { return x + w; }
  double bottom() const noexcept { return y + h; }

  bool empty() const noexcept { return w <= 0 || h <= 0; }

  bool intersects(RectF const& o) const noexcept
  {
    return x < o.right() && right() > o.x && y < o.bottom() && bottom() > o.y;
  }

  RectF intersection(RectF const& o) const noexcept
  {
    double const nx = x > o.x ? x : o.x;
    double const ny = y > o.y ? y : o.y;
    double const nr = right() < o.right() ? right() : o.right();
    double const nb = bottom() < o.bottom() ? bottom() : o.bottom();
    if (nr <= nx || nb <= ny) {
      return {};
    }
    return {nx, ny, nr - nx, nb - ny};
  }
};

/// Integer content-space rect (pixel-aligned content coordinates).
struct RectI {
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;

  int right() const noexcept { return x + w; }
  int bottom() const noexcept { return y + h; }
  bool empty() const noexcept { return w <= 0 || h <= 0; }
};

/// Viewport in content space plus screen density.
struct Viewport {
  RectF content_rect;  ///< Visible region in scale-0 content pixels
  /// Device (screen) pixels per content pixel along the longer mapped axis.
  /// 1.0 = 1:1; 2.0 = zoomed in 2×; 0.5 = zoomed out.
  double device_per_content = 1.0;
};

/// Owning pixel buffer for one tile (decoded RGBA8 or encoded payload).
struct TileBitmap {
  int width = 0;
  int height = 0;
  /// RGBA8 row-major when codec empty or "rgba8"; otherwise opaque bytes.
  std::string codec;  ///< e.g. "" / "rgba8" / "jpeg"
  std::vector<std::uint8_t> bytes;

  bool valid() const noexcept
  {
    return width > 0 && height > 0 && !bytes.empty();
  }
};

/**
 * Explicit per-cell load state, owned by the path's TileLoader.
 * A key with no entry is **Missing**. See docs/TILE_STATE_MACHINE.md.
 *
 *   Missing ──issue──▶ Queued ──Ok──▶ Ready
 *                        │  ├─Cancelled──▶ Missing
 *                        │  ├─Failed────▶ Failed (retry_at, attempts++)
 *                        │  ├─Unavailable▶ Unavailable (permanent)
 *                        │  └─stall timeout▶ Failed ("no reply after …")
 *   Failed ──retry due & demanded──▶ Queued
 */
enum class CellState : std::uint8_t {
  Queued,       ///< Exactly one request outstanding at the backend (ticket)
  Ready,        ///< Pixels present
  Failed,       ///< Last attempt failed; retried at retry_at while demanded
  Unavailable,  ///< Backend says the cell cannot exist; never retried
};

[[nodiscard]] constexpr char const* cell_state_name(CellState s) noexcept
{
  switch (s) {
  case CellState::Queued:
    return "queued";
  case CellState::Ready:
    return "ready";
  case CellState::Failed:
    return "failed";
  case CellState::Unavailable:
    return "unavailable";
  }
  return "?";
}

struct TileCell {
  CellState state = CellState::Queued;
  TileBitmap bitmap;            ///< Ready only
  std::uint64_t ticket = 0;     ///< Queued: id of the outstanding request
  int attempts = 0;             ///< Finished Failed attempts since last success
  std::string error;            ///< Failed / Unavailable: reason (backend text)
  std::int64_t since_ms = 0;    ///< Loader clock when the state was entered
  std::int64_t retry_at_ms = 0; ///< Failed: earliest re-issue
  std::uint64_t last_used = 0;  ///< Loader-wide LRU clock (Ready eviction)

  bool ready() const noexcept
  {
    return state == CellState::Ready && bitmap.valid();
  }
};

enum class DrawKind {
  ExactTile,     ///< Cache hit at target scale
  CoarserTile,   ///< Parent stand-in
  Underlay,      ///< LQIP / overview (host paints separately if needed)
  Empty          ///< Nothing available
};

/// One paint operation in content space.
struct DrawCommand {
  RectF dst_content;   ///< Where to place in content coordinates
  TileKey src_key;     ///< Tile that supplies pixels (if any)
  RectF src_uv;        ///< Sub-rect in tile pixel space [0,width]×[0,height]
  DrawKind kind = DrawKind::Empty;
  /// True when kind is Underlay and host should use session LQIP for this rect.
  bool use_lqip = false;
};

struct DrawPlan {
  std::vector<DrawCommand> commands;
  int target_scale = 0;
  bool any_tile = false;  ///< At least one ExactTile or CoarserTile
};

}  // namespace thumtoo::lod

