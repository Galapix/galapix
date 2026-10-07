// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Linked only when THUMTOO_HAVE_MUPDF is unset (see CMakeLists.txt).

#include "thumtoo/pdf_mupdf.hpp"

namespace thumtoo {

void mupdf_force_next_open_as_text(const std::filesystem::path& path) {
  (void)path;
}

std::string mupdf_last_error() { return {}; }

void mupdf_clear_last_error() {}

std::optional<int> mupdf_page_count(const std::filesystem::path& path) {
  (void)path;
  return std::nullopt;
}

std::optional<Size> mupdf_page_size_72dpi(const std::filesystem::path& path,
                                          int page_1based) {
  (void)path;
  (void)page_1based;
  return std::nullopt;
}

std::optional<Size> mupdf_page_layout_size(const std::filesystem::path& path,
                                            int page_1based) {
  (void)path;
  (void)page_1based;
  return std::nullopt;
}

std::optional<PdfPageProfile> mupdf_page_profile(const std::filesystem::path&, int,
                                                 std::string* error) {
  if (error) *error = "PDF backend unavailable";
  return std::nullopt;
}

std::optional<std::string> mupdf_scale_refusal(const std::filesystem::path&, int, int) {
  return std::string("PDF backend unavailable");
}

std::optional<PdfDocumentRenderStats> mupdf_document_render_stats(
    const std::filesystem::path&) {
  return std::nullopt;
}

void mupdf_reset_render_stats() {}

void pdf_set_full_image_decode_budget(std::size_t) {}
void pdf_release_document_cache() {}
std::size_t pdf_full_image_decode_budget() { return kPdfFullImageDecodeBudget; }

std::optional<PdfRaster> mupdf_rasterize_page(const std::filesystem::path& path,
                                               int page_1based, int max_edge) {
  (void)path;
  (void)page_1based;
  (void)max_edge;
  return std::nullopt;
}

std::optional<PdfRaster> mupdf_rasterize_page_region(
    const std::filesystem::path& path, int page_1based, double dpi, int px,
    int py, int pw, int ph) {
  (void)path;
  (void)page_1based;
  (void)dpi;
  (void)px;
  (void)py;
  (void)pw;
  (void)ph;
  return std::nullopt;
}

PdfCellRender mupdf_render_tile_cell(const std::filesystem::path&, int, int, int,
                                     int) {
  return {TileStatus::Failed, std::nullopt, "PDF backend unavailable"};
}

std::optional<int> mupdf_embedded_image_count(const std::filesystem::path& path) {
  (void)path;
  return std::nullopt;
}

std::optional<PdfRaster> mupdf_rasterize_embedded_image(
    const std::filesystem::path& path, int image_1based, int max_edge) {
  (void)path;
  (void)image_1based;
  (void)max_edge;
  return std::nullopt;
}

std::optional<Size> mupdf_embedded_image_size(const std::filesystem::path& path,
                                               int image_1based) {
  (void)path;
  (void)image_1based;
  return std::nullopt;
}

std::optional<PageTextLayer> mupdf_page_text_layer(
    const std::filesystem::path& path, int page_1based) {
  (void)path;
  (void)page_1based;
  return std::nullopt;
}

std::optional<DocumentOutline> mupdf_document_outline(
    const std::filesystem::path& path) {
  (void)path;
  return std::nullopt;
}

std::optional<PdfRaster> mupdf_page_thumb_rgb(const std::filesystem::path& path,
                                              int page_1based) {
  (void)path;
  (void)page_1based;
  return std::nullopt;
}

}  // namespace thumtoo

namespace thumtoo {

void set_smooth_image_scaling(bool on) { (void)on; }
bool smooth_image_scaling() { return true; }

}  // namespace thumtoo
