// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/constants.hpp"
#include "thumtoo/pdf.hpp"  // page profile / render stats types (shared by both backends)
#include "thumtoo/types.hpp"
#include "thumtoo/text.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace thumtoo {

struct ParsedDjvuUri {
  std::filesystem::path djvu_path;
  /// 1-based page index (matches //page:N).
  int page = 0;
};

/// file:///abs/doc.djvu//page:12  (page is 1-based; same pipe as PDF).
[[nodiscard]] std::string djvu_page_uri(const std::filesystem::path& djvu_path,
                                        int page_1based);

[[nodiscard]] std::optional<ParsedDjvuUri> parse_djvu_uri(std::string_view uri);

[[nodiscard]] bool is_likely_djvu_path(const std::filesystem::path& path);

[[nodiscard]] std::optional<int> djvu_page_count(const std::filesystem::path& path);

struct DjvuRaster {
  int width = 0;
  int height = 0;
  /// Contiguous RGB888 rows (no padding).
  std::vector<std::uint8_t> rgb;
};

[[nodiscard]] std::optional<DjvuRaster> djvu_rasterize_page(
    const std::filesystem::path& path, int page_1based, int max_edge);

/// Native page size in pixels (decoder units), page 1-based.
[[nodiscard]] std::optional<Size> djvu_page_size_native(
    const std::filesystem::path& path, int page_1based);

/// Layout size for Galapix-style tiles (same DPI convention as PDF).
[[nodiscard]] std::optional<Size> djvu_page_layout_size(
    const std::filesystem::path& path, int page_1based);

/// Page pixel size at tile scale @p scale. Layout (scale 0) is the page's
/// native pixel grid. Coarser scales floor-halve per step (dim_at_tile_scale,
/// the host's grid); denser scales multiply exactly.
[[nodiscard]] Size djvu_page_size_at_scale(Size layout, int scale);

/**
 * Rasterize a pixel rectangle of the page scaled by @p scale_factor
 * (native pixels × factor). Reuses the decoded page.
 */
[[nodiscard]] std::optional<DjvuRaster> djvu_rasterize_page_region(
    const std::filesystem::path& path, int page_1based, double scale_factor, int px,
    int py, int pw, int ph);

/// Outcome of one live cell render: pixels, or a status with the reason.
struct DjvuCellRender {
  TileStatus status = TileStatus::Failed;
  std::optional<DjvuRaster> raster;
  std::string error;
};

/**
 * One tile cell. Scales finer than 0 are Unavailable: layout already is the
 * page's native pixel grid (a DjVu page is a raster), so denser cells would
 * only interpolate.
 */
[[nodiscard]] DjvuCellRender djvu_render_tile_cell(
    const std::filesystem::path& path, int page_1based, int scale, int x, int y);

[[nodiscard]] std::optional<TileBlob> djvu_build_tile_cell(
    const std::filesystem::path& path, int page_1based, int scale, int x,
    int y, int jpeg_quality = kDefaultTileQuality);

/**
 * Page profile from the decoded page: always Raster (or Empty for a page
 * without image layers); `images` lists the JB2 mask / IW44 layers with their
 * resolution, `invisible_glyphs` counts the hidden text layer, native_dpi is
 * the page dpi and finest_useful_scale is 0 (native pixels).
 */
[[nodiscard]] std::optional<PdfPageProfile> djvu_page_profile(
    const std::filesystem::path& path, int page_1based, std::string* error = nullptr);

/// Decode / render accounting (same structure as PDF; `decode` counts page
/// decodes, display_list_builds stays 0). Cheap; any thread.
[[nodiscard]] std::optional<PdfDocumentRenderStats> djvu_document_render_stats(
    const std::filesystem::path& path);
void djvu_reset_render_stats();

/// Close every cached DjVu document (memory pressure, leak-checked shutdown).
void djvu_release_document_cache();

/**
 * Hidden text layer + hyperlink mapareas for one DjVu page.
 * Bboxes are in native page pixels, origin bottom-left (DjVu default),
 * same space as djvu_page_size_native. Empty regions if no OCR layer.
 */
[[nodiscard]] std::optional<PageTextLayer> djvu_page_text_layer(
    const std::filesystem::path& path, int page_1based);

/// Document outline / bookmarks when present (NAVM); empty list if none.
[[nodiscard]] std::optional<DocumentOutline> djvu_document_outline(
    const std::filesystem::path& path);

}  // namespace thumtoo
