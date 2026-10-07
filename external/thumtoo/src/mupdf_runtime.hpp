// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Internal: the shared MuPDF runtime of pdf_mupdf.cpp (one store, cloned
// contexts, cached documents with display lists, page profiles, decode-once
// images, render stats) for the other MuPDF-backed formats (EPUB).
// Not installed; MuPDF builds only.

#include "thumtoo/pdf.hpp"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>

#include <mupdf/fitz.h>

namespace thumtoo {

void utf8_append_codepoint(std::string& out, int c);
[[nodiscard]] bool is_external_link_uri(const char* uri);
[[nodiscard]] bool resolve_internal_link_page(fz_context* ctx, fz_document* doc,
                                              const char* uri, int* page_0based,
                                              float* x_out, float* y_out);

namespace mupdf {

/// How a reflowable document is opened: user CSS and the page box it is laid
/// out into. Documents with different specs are cached separately.
struct OpenSpec {
  std::string key;  ///< canonical layout parameters (cache key suffix)
  std::string css;  ///< fz_style_document user CSS
  bool use_document_css = true;
  float width_pt = 0.0f;
  float height_pt = 0.0f;
  float em = 0.0f;
};

// All functions: @p spec may be null (fixed-layout documents). Thread-safe.
// Pages are 1-based; layout size is at kPdfLayoutDpi (== kEpubLayoutDpi).

[[nodiscard]] std::optional<int> page_count(const std::filesystem::path& path,
                                            const OpenSpec* spec,
                                            std::string* error = nullptr);
[[nodiscard]] std::optional<Size> page_layout_size(const std::filesystem::path& path,
                                                   const OpenSpec* spec, int page);
[[nodiscard]] std::optional<PdfPageProfile> page_profile(const std::filesystem::path& path,
                                                         const OpenSpec* spec, int page,
                                                         std::string* error);
[[nodiscard]] PdfCellRender render_cell(const std::filesystem::path& path,
                                        const OpenSpec* spec, int page, int scale, int x,
                                        int y);
[[nodiscard]] std::optional<PdfRaster> rasterize_page(const std::filesystem::path& path,
                                                      const OpenSpec* spec, int page,
                                                      int max_edge);
[[nodiscard]] std::optional<PdfRaster> rasterize_region(const std::filesystem::path& path,
                                                        const OpenSpec* spec, int page,
                                                        double dpi, int px, int py, int pw,
                                                        int ph);

/// Run @p fn with exclusive access to the document and, when @p page >= 1,
/// its loaded page (else nullptr). @return false (with @p error) when the
/// document or page cannot be opened.
bool with_document(const std::filesystem::path& path, const OpenSpec* spec, int page,
                   const std::function<void(fz_context*, fz_document*, fz_page*)>& fn,
                   std::string* error = nullptr);

}  // namespace mupdf
}  // namespace thumtoo
