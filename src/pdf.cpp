// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/pdf.hpp"

#include <cstdio>
#include "thumtoo/pdf_mupdf.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/format.hpp"
#include "thumtoo/uri.hpp"
#include "thumtoo/image.hpp"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>

namespace thumtoo {
namespace {

constexpr std::string_view kPagePipe = "//page:";
/// Legacy //poppler-page: still accepted, treated as //page: (MuPDF).
constexpr std::string_view kPopplerPagePipe = "//poppler-page:";
constexpr std::string_view kMupdfPagePipe = "//mupdf-page:";

}  // namespace

const char* page_content_kind_name(PageContentKind kind) {
  switch (kind) {
    case PageContentKind::Empty: return "empty";
    case PageContentKind::Vector: return "vector";
    case PageContentKind::Raster: return "raster";
    case PageContentKind::Mixed: return "mixed";
  }
  return "?";
}

bool is_likely_pdf_path(const std::filesystem::path& path) {
  return is_pdf_path(path);
}

PdfBackend pdf_resolve_backend(PdfBackend requested) {
  (void)requested;
#if defined(THUMTOO_HAVE_MUPDF)
  // All PdfBackend values resolve to MuPDF.
  return PdfBackend::MuPDF;
#else
  return PdfBackend::Default;
#endif
}

bool pdf_backend_available(PdfBackend backend) {
  (void)backend;
#if defined(THUMTOO_HAVE_MUPDF)
  return true;
#else
  return false;
#endif
}

const char* pdf_backend_name(PdfBackend backend) {
  (void)backend;
#if defined(THUMTOO_HAVE_MUPDF)
  return "mupdf";
#else
  return "none";
#endif
}

std::string pdf_image_uri(const std::filesystem::path& pdf_path, int image_1based) {
  std::string uri = file_uri_from_path(pdf_path.lexically_normal());
  uri += "//pdfimage:";
  uri += std::to_string(std::max(1, image_1based));
  return uri;
}

std::optional<ParsedPdfImageUri> parse_pdf_image_uri(std::string_view uri) {
  constexpr std::string_view kPipe = "//pdfimage:";
  auto pos = uri.find(kPipe);
  if (pos == std::string_view::npos) return std::nullopt;
  const auto outer = uri.substr(0, pos);
  auto path = path_from_file_uri(outer);
  if (!path) {
    if (outer.find("://") != std::string_view::npos) return std::nullopt;
    path = std::filesystem::path(std::string(outer));
  }
  if (!is_pdf_path(*path) && !is_likely_pdf_path(*path)) return std::nullopt;
  std::string_view rest = uri.substr(pos + kPipe.size());
  if (rest.empty()) return std::nullopt;
  int image = 0;
  for (char c : rest) {
    if (c < '0' || c > '9') break;
    image = image * 10 + (c - '0');
    if (image > 1'000'000) return std::nullopt;
  }
  if (image < 1) return std::nullopt;
  ParsedPdfImageUri out;
  out.pdf_path = *path;
  out.image = image;
  return out;
}

std::optional<int> pdf_embedded_image_count(const std::filesystem::path& path) {
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_embedded_image_count(path);
#else
  (void)path;
  return std::nullopt;
#endif
}

std::optional<PdfRaster> pdf_rasterize_embedded_image(const std::filesystem::path& path,
                                                      int image_1based, int max_edge) {
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_rasterize_embedded_image(path, image_1based, max_edge);
#else
  (void)path;
  (void)image_1based;
  (void)max_edge;
  return std::nullopt;
#endif
}

std::optional<Size> pdf_embedded_image_size(const std::filesystem::path& path,
                                            int image_1based) {
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_embedded_image_size(path, image_1based);
#else
  (void)path;
  (void)image_1based;
  return std::nullopt;
#endif
}

std::string pdf_page_uri(const std::filesystem::path& pdf_path, int page_1based,
                         PdfBackend backend) {
  auto uri = file_uri_from_path(pdf_path.lexically_normal());
  // Prefer neutral //page:N. Explicit MuPDF only when requested.
  if (backend == PdfBackend::MuPDF) {
    uri += "//mupdf-page:";
  } else {
    uri += "//page:";
  }
  uri += std::to_string(std::max(1, page_1based));
  return uri;
}

std::optional<ParsedPdfUri> parse_pdf_uri(std::string_view uri) {
  PdfBackend backend = PdfBackend::Default;
  std::string_view tag = kPagePipe;
  auto pipe = uri.find(kPagePipe);
  auto pop = uri.find(kPopplerPagePipe);
  auto mu = uri.find(kMupdfPagePipe);

  std::size_t pos = std::string_view::npos;
  auto consider = [&](std::size_t p, PdfBackend b, std::string_view t) {
    if (p == std::string_view::npos) return;
    if (pos == std::string_view::npos || p < pos) {
      pos = p;
      backend = b;
      tag = t;
    }
  };
  consider(pipe, PdfBackend::Default, kPagePipe);
  // Legacy //poppler-page: same as //page: (MuPDF).
  consider(pop, PdfBackend::Default, kPopplerPagePipe);
  consider(mu, PdfBackend::MuPDF, kMupdfPagePipe);
  if (pos == std::string_view::npos) return std::nullopt;

  const auto outer = uri.substr(0, pos);
  auto path = path_from_file_uri(outer);
  if (!path) {
    if (!outer.empty() && outer.front() == '/') {
      path = std::filesystem::path(std::string(outer));
    }
  }
  if (!path) return std::nullopt;

  // Optional //text force pipe before or after page tag.
  bool force_text = uri_has_text_force_pipe(uri);
  if (force_text) {
    // Outer may still include //text if page pipe was found after it.
    std::string path_str = path->string();
    auto tpos = path_str.find("//text");
    if (tpos != std::string::npos) {
      path_str.resize(tpos);
      *path = std::filesystem::path(path_str);
    }
  }

  // PDF / Markdown / plain text, or any path with //text force.
  if (!force_text && !is_mupdf_page_document_path(*path)) return std::nullopt;

  std::string_view rest = uri.substr(pos + tag.size());
  if (rest.empty()) return std::nullopt;
  int page = 0;
  for (char c : rest) {
    if (c < '0' || c > '9') return std::nullopt;
    page = page * 10 + (c - '0');
    if (page > 1'000'000) return std::nullopt;
  }
  if (page < 1) return std::nullopt;

  ParsedPdfUri out;
  out.pdf_path = *path;
  out.page = page;
  out.backend = backend;
  out.force_text = force_text;
  if (force_text) {
    mupdf_force_next_open_as_text(out.pdf_path);
  }
  return out;
}

std::optional<int> pdf_page_count(const std::filesystem::path& path,
                                  PdfBackend backend) {
  (void)backend;
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_page_count(path);
#else
  (void)path;
  return std::nullopt;
#endif
}

std::optional<Size> pdf_page_size_72dpi(const std::filesystem::path& path,
                                        int page_1based, PdfBackend backend) {
  (void)backend;
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_page_size_72dpi(path, page_1based);
#else
  (void)path;
  (void)page_1based;
  return std::nullopt;
#endif
}

std::optional<Size> pdf_page_layout_size(const std::filesystem::path& path,
                                         int page_1based, PdfBackend backend) {
  (void)backend;
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_page_layout_size(path, page_1based);
#else
  (void)path;
  (void)page_1based;
  return std::nullopt;
#endif
}

Size pdf_page_size_at_scale(Size layout, int scale) {
  if (layout.width <= 0 || layout.height <= 0) return Size{0, 0};
  // Positive scale: successive floor-half (same as dim_at_tile_scale / image
  // pyramid). lround(layout * 2^-s) drifted from the host tile grid on
  // right/bottom edge cells.
  // Negative scale: exact integer expand layout * 2^{-s} (dim_at_tile_scale is
  // a no-op for scale <= 0 and must not be used here).
  if (scale < 0) {
    const int mul = 1 << (-scale);
    return Size{layout.width * mul, layout.height * mul};
  }
  return Size{dim_at_tile_scale(layout.width, scale),
              dim_at_tile_scale(layout.height, scale)};
}

double pdf_dpi_for_scale(int scale) {
  return static_cast<double>(kPdfLayoutDpi) * std::ldexp(1.0, -scale);
}

std::optional<PdfRaster> pdf_rasterize_page(const std::filesystem::path& path,
                                            int page_1based, int max_edge,
                                            PdfBackend backend) {
  (void)backend;
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_rasterize_page(path, page_1based, max_edge);
#else
  (void)path;
  (void)page_1based;
  (void)max_edge;
  return std::nullopt;
#endif
}

std::optional<PdfPageProfile> pdf_page_profile(const std::filesystem::path& path,
                                               int page_1based, std::string* error) {
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_page_profile(path, page_1based, error);
#else
  (void)path;
  (void)page_1based;
  if (error) *error = "PDF backend unavailable";
  return std::nullopt;
#endif
}

std::optional<std::string> pdf_scale_refusal(const std::filesystem::path& path,
                                             int page_1based, int scale) {
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_scale_refusal(path, page_1based, scale);
#else
  (void)path;
  (void)page_1based;
  (void)scale;
  return std::string("PDF backend unavailable");
#endif
}

std::optional<PdfDocumentRenderStats> pdf_document_render_stats(
    const std::filesystem::path& path) {
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_document_render_stats(path);
#else
  (void)path;
  return std::nullopt;
#endif
}

void pdf_reset_render_stats() {
#if defined(THUMTOO_HAVE_MUPDF)
  mupdf_reset_render_stats();
#endif
}

std::optional<PdfRaster> pdf_rasterize_page_region(
    const std::filesystem::path& path, int page_1based, double dpi, int px,
    int py, int pw, int ph, PdfBackend backend) {
  (void)backend;
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_rasterize_page_region(path, page_1based, dpi, px, py, pw, ph);
#else
  (void)path;
  (void)page_1based;
  (void)dpi;
  (void)px;
  (void)py;
  (void)pw;
  (void)ph;
  return std::nullopt;
#endif
}

PdfCellRender pdf_render_tile_cell(const std::filesystem::path& path,
                                   int page_1based, int scale, int x, int y,
                                   PdfBackend backend) {
  (void)backend;
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_render_tile_cell(path, page_1based, scale, x, y);
#else
  (void)path;
  (void)page_1based;
  (void)scale;
  (void)x;
  (void)y;
  return {TileStatus::Failed, std::nullopt, "PDF backend unavailable"};
#endif
}

std::optional<TileBlob> pdf_build_tile_cell(const std::filesystem::path& path,
                                            int page_1based, int scale, int x,
                                            int y, int jpeg_quality,
                                            PdfBackend backend) {
  auto cell = pdf_render_tile_cell(path, page_1based, scale, x, y, backend);
  if (!cell.raster || cell.raster->rgb.empty()) return std::nullopt;
  return encode_tile_cell_rgb(cell.raster->rgb.data(), cell.raster->width,
                              cell.raster->height, scale, x, y, jpeg_quality);
}

std::optional<PageTextLayer> pdf_page_text_layer(const std::filesystem::path& path,
                                                 int page_1based,
                                                 PdfBackend backend) {
  (void)backend;
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_page_text_layer(path, page_1based);
#else
  (void)path;
  (void)page_1based;
  return std::nullopt;
#endif
}

std::optional<DocumentOutline> pdf_document_outline(const std::filesystem::path& path,
                                                    PdfBackend backend) {
  (void)backend;
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_document_outline(path);
#else
  (void)path;
  return std::nullopt;
#endif
}

std::optional<PdfRaster> pdf_page_thumb_rgb(const std::filesystem::path& path,
                                            int page_1based, PdfBackend backend) {
  (void)backend;
#if defined(THUMTOO_HAVE_MUPDF)
  return mupdf_page_thumb_rgb(path, page_1based);
#else
  (void)path;
  (void)page_1based;
  return std::nullopt;
#endif
}

}  // namespace thumtoo
