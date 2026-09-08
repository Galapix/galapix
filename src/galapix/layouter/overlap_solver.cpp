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

#include "galapix/layouter/overlap_solver.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include <logmich/log.hpp>

#include "galapix/workspace_item.hpp"

namespace galapix {

namespace {

struct Body
{
  float cx = 0.0f;
  float cy = 0.0f;
  float hw = 0.0f; // half width
  float hh = 0.0f; // half height
};

struct PairHash
{
  std::size_t operator()(std::pair<int, int> const& p) const noexcept
  {
    return (static_cast<std::size_t>(static_cast<uint32_t>(p.first)) << 32) ^
           static_cast<std::size_t>(static_cast<uint32_t>(p.second));
  }
};

using CellKey = std::int64_t;

CellKey cell_key(int gx, int gy)
{
  return (static_cast<CellKey>(static_cast<uint32_t>(gx)) << 32) |
         static_cast<CellKey>(static_cast<uint32_t>(gy));
}

float median_extent(std::vector<Body> const& bodies)
{
  if (bodies.empty()) {
    return 256.0f;
  }
  std::vector<float> sizes;
  sizes.reserve(bodies.size());
  for (auto const& b : bodies) {
    sizes.push_back(std::max(b.hw, b.hh) * 2.0f);
  }
  std::nth_element(sizes.begin(), sizes.begin() + static_cast<std::ptrdiff_t>(sizes.size() / 2),
                   sizes.end());
  float const m = sizes[sizes.size() / 2];
  return std::max(m, 1.0f);
}

/** Resolve one pair along the minimum penetration axis. Returns true if they overlapped. */
bool separate_pair(Body& a, Body& b, float gap)
{
  float const dx = b.cx - a.cx;
  float const dy = b.cy - a.cy;
  float const ox = a.hw + b.hw + gap - std::fabs(dx);
  float const oy = a.hh + b.hh + gap - std::fabs(dy);
  if (ox <= 0.0f || oy <= 0.0f) {
    return false;
  }

  // Prefer the axis of least penetration (minimum translation).
  if (ox < oy) {
    float const push = ox * 0.5f;
    if (dx >= 0.0f) {
      a.cx -= push;
      b.cx += push;
    } else {
      a.cx += push;
      b.cx -= push;
    }
  } else {
    float const push = oy * 0.5f;
    if (dy >= 0.0f) {
      a.cy -= push;
      b.cy += push;
    } else {
      a.cy += push;
      b.cy -= push;
    }
  }
  return true;
}

} // namespace

int
solve_image_overlaps(ImageCollection& images, float gap)
{
  int const n = static_cast<int>(images.size());
  if (n < 2) {
    return 0;
  }

  std::vector<Body> bodies(static_cast<std::size_t>(n));
  for (int i = 0; i < n; ++i) {
    WorkspaceItemPtr const& item = images[static_cast<ImageCollection::size_type>(i)];
    Vector2f const pos = item->get_pos();
    float const w = item->get_scaled_width();
    float const h = item->get_scaled_height();
    bodies[static_cast<std::size_t>(i)].cx = pos.x();
    bodies[static_cast<std::size_t>(i)].cy = pos.y();
    bodies[static_cast<std::size_t>(i)].hw = std::max(w * 0.5f, 0.5f);
    bodies[static_cast<std::size_t>(i)].hh = std::max(h * 0.5f, 0.5f);
  }

  float const cell = median_extent(bodies);
  constexpr int kMaxIters = 64;
  int iters = 0;
  int last_pairs = 0;

  for (; iters < kMaxIters; ++iters) {
    std::unordered_map<CellKey, std::vector<int>> grid;
    grid.reserve(static_cast<std::size_t>(n) * 2u);

    for (int i = 0; i < n; ++i) {
      Body const& b = bodies[static_cast<std::size_t>(i)];
      int const x0 = static_cast<int>(std::floor((b.cx - b.hw) / cell));
      int const x1 = static_cast<int>(std::floor((b.cx + b.hw) / cell));
      int const y0 = static_cast<int>(std::floor((b.cy - b.hh) / cell));
      int const y1 = static_cast<int>(std::floor((b.cy + b.hh) / cell));
      for (int gy = y0; gy <= y1; ++gy) {
        for (int gx = x0; gx <= x1; ++gx) {
          grid[cell_key(gx, gy)].push_back(i);
        }
      }
    }

    std::unordered_map<std::pair<int, int>, bool, PairHash> seen;
    seen.reserve(static_cast<std::size_t>(n));
    int resolved = 0;

    for (auto const& entry : grid) {
      std::vector<int> const& bucket = entry.second;
      int const m = static_cast<int>(bucket.size());
      for (int a = 0; a < m; ++a) {
        for (int b = a + 1; b < m; ++b) {
          int i = bucket[static_cast<std::size_t>(a)];
          int j = bucket[static_cast<std::size_t>(b)];
          if (i > j) {
            std::swap(i, j);
          }
          if (i == j) {
            continue;
          }
          auto const key = std::make_pair(i, j);
          if (!seen.emplace(key, true).second) {
            continue;
          }
          if (separate_pair(bodies[static_cast<std::size_t>(i)],
                            bodies[static_cast<std::size_t>(j)], gap)) {
            ++resolved;
          }
        }
      }
    }

    last_pairs = resolved;
    if (resolved == 0) {
      break;
    }
  }

  for (int i = 0; i < n; ++i) {
    Body const& b = bodies[static_cast<std::size_t>(i)];
    images[static_cast<ImageCollection::size_type>(i)]->set_pos(
      Vector2f(b.cx, b.cy));
  }

  if (iters >= kMaxIters && last_pairs > 0) {
    log_info("overlap solver: stopped after {} iterations ({} pairs still overlapping)",
             iters, last_pairs);
  } else {
    log_debug("overlap solver: {} image(s), {} iteration(s)", n, iters);
  }

  return iters;
}

} // namespace galapix

/* EOF */
