// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/constants.hpp"
#include "thumtoo/pdf.hpp"
#include "thumtoo/types.hpp"
#include "thumtoo/uri.hpp"
#include "thumtoo/text.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace thumtoo {

struct ParsedEpubUri {
  std::filesystem::path epub_path;
  EpubLayout layout = default_epub_layout();
  int page = 1;  // 1-based
};

/// True when THUMTOO_HAVE_MUPDF and path looks like .epub.
[[nodiscard]] bool epub_available();

/// Extension heuristic (.epub), independent of MuPDF build.
[[nodiscard]] bool is_likely_epub_path(const std::filesystem::path& path);

[[nodiscard]] std::optional<ParsedEpubUri> parse_epub_uri(std::string_view uri);

/// file:///…//epub:w=…,h=…,fs=…//page:N
[[nodiscard]] std::string epub_page_uri(const std::filesystem::path& path,
                                        int page_1based,
                                        const EpubLayout& layout = default_epub_layout());

/// Layout then count pages. nullopt if MuPDF missing / open/layout fails.
[[nodiscard]] std::optional<int> epub_page_count(const std::filesystem::path& path,
                                                 const EpubLayout& layout = default_epub_layout());

/// Page pixel size at kEpubLayoutDpi (Galapix scale-0).
[[nodiscard]] std::optional<Size> epub_page_layout_size(
    const std::filesystem::path& path, int page_1based,
    const EpubLayout& layout = default_epub_layout());

/// Full-page raster; long edge ≤ max_edge (overview / ladder).
[[nodiscard]] std::optional<PdfRaster> epub_rasterize_page(
    const std::filesystem::path& path, int page_1based, const EpubLayout& layout,
    int max_edge);

[[nodiscard]] std::optional<PdfRaster> epub_rasterize_page_region(
    const std::filesystem::path& path, int page_1based, const EpubLayout& layout,
    double dpi, int px, int py, int pw, int ph);

[[nodiscard]] std::optional<PdfRaster> epub_render_tile_cell(
    const std::filesystem::path& path, int page_1based, const EpubLayout& layout,
    int scale, int x, int y);

/**
 * Text + link regions for one laid-out EPUB page (MuPDF).
 * layout_key is format_epub_layout_params(layout). Geometry is invalid when
 * layout changes — regenerate. Bboxes in page space (points, Y up),
 * same as fz_bound_page after layout.
 */
[[nodiscard]] std::optional<PageTextLayer> epub_page_text_layer(
    const std::filesystem::path& path, int page_1based,
    const EpubLayout& layout = default_epub_layout());

/// NAV / spine outline when MuPDF exposes one after open+layout.
[[nodiscard]] std::optional<DocumentOutline> epub_document_outline(
    const std::filesystem::path& path,
    const EpubLayout& layout = default_epub_layout());

}  // namespace thumtoo
