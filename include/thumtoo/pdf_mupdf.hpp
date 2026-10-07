// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/pdf.hpp"
#include "thumtoo/text.hpp"

#include <filesystem>
#include <optional>

namespace thumtoo {

/// MuPDF backend (compiled only usefully when THUMTOO_HAVE_MUPDF).
/// Public pdf_* APIs dispatch here when PdfBackend resolves to MuPDF.

/// Next tls_document open for this path uses MuPDF filetype magic "txt"
/// (for //text force or non-native text extensions). Thread-local.
void mupdf_force_next_open_as_text(const std::filesystem::path& path);

/// Last MuPDF error/warning line captured on this thread (empty if none).
[[nodiscard]] std::string mupdf_last_error();
void mupdf_clear_last_error();

[[nodiscard]] std::optional<int> mupdf_page_count(const std::filesystem::path& path);

[[nodiscard]] std::optional<Size> mupdf_page_size_72dpi(const std::filesystem::path& path,
                                                        int page_1based);

/// Layout pixels at kPdfLayoutDpi: one lround from continuous page bounds.
/// Do not route through integer 72dpi size (double-round drifts by up to 1px).
[[nodiscard]] std::optional<Size> mupdf_page_layout_size(
    const std::filesystem::path& path, int page_1based);

[[nodiscard]] std::optional<PdfPageProfile> mupdf_page_profile(
    const std::filesystem::path& path, int page_1based, std::string* error);

[[nodiscard]] std::optional<std::string> mupdf_scale_refusal(
    const std::filesystem::path& path, int page_1based, int scale);

[[nodiscard]] std::optional<PdfDocumentRenderStats> mupdf_document_render_stats(
    const std::filesystem::path& path);

void mupdf_reset_render_stats();

[[nodiscard]] std::optional<PdfRaster> mupdf_rasterize_page(
    const std::filesystem::path& path, int page_1based, int max_edge);

[[nodiscard]] std::optional<PdfRaster> mupdf_rasterize_page_region(
    const std::filesystem::path& path, int page_1based, double dpi, int px,
    int py, int pw, int ph);

[[nodiscard]] PdfCellRender mupdf_render_tile_cell(
    const std::filesystem::path& path, int page_1based, int scale, int x,
    int y);

[[nodiscard]] std::optional<int> mupdf_embedded_image_count(
    const std::filesystem::path& path);

[[nodiscard]] std::optional<PdfRaster> mupdf_rasterize_embedded_image(
    const std::filesystem::path& path, int image_1based, int max_edge);

[[nodiscard]] std::optional<Size> mupdf_embedded_image_size(
    const std::filesystem::path& path, int image_1based);

/// Structured text + link regions for one page (MuPDF).
/// Bboxes are in page space (points, same as fz_bound_page / media box).
/// Y increases upward (PDF user space). Reading order is preserved.
[[nodiscard]] std::optional<PageTextLayer> mupdf_page_text_layer(
    const std::filesystem::path& path, int page_1based);

/// Document outline / bookmarks (flattened with level).
[[nodiscard]] std::optional<DocumentOutline> mupdf_document_outline(
    const std::filesystem::path& path);

/// Page dictionary /Thumb stream as RGB888 when present (no full page render).
[[nodiscard]] std::optional<PdfRaster> mupdf_page_thumb_rgb(
    const std::filesystem::path& path, int page_1based);

}  // namespace thumtoo
