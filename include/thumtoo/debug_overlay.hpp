// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/types.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace thumtoo {

/**
 * THUMTOO_DEBUG_OVERLAY=1 (or non-empty non-0): stamp returned PixelLevel and
 * TileBlob rasters in **source** pixel space (magenta border + large centred
 * text). Soft: SOFT / le=N. Tile: TILE / s=N / x,y. Hosts orient/flip the
 * bitmap — stamps follow automatically. Off by default; applied on read path
 * only (durable Store bytes unchanged).
 *
 * Text uses an embedded 8×12 bitmap font (no fontconfig/Pango) so worker
 * threads stay self-contained and labels stay readable when scaled up.
 */
[[nodiscard]] bool debug_overlay_enabled();

/** Stamp in-place RGB888 (tight row, 3 bytes/pixel). Draws border + text lines. */
void debug_overlay_rgb888(std::uint8_t* rgb, int width, int height,
                          const std::vector<std::string>& lines);

/** Decode → stamp → re-encode a returned soft level (JXL/JPEG). No-op if disabled. */
void debug_overlay_pixel_level(PixelLevel& px, std::string_view uri_tail,
                               int request_edge);

/** Decode → stamp → re-encode a returned grid tile. No-op if disabled. */
void debug_overlay_tile(TileBlob& tile, std::string_view uri_tail);

}  // namespace thumtoo
