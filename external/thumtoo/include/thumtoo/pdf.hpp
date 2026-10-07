// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/constants.hpp"
#include "thumtoo/types.hpp"
#include "thumtoo/text.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace thumtoo {

/// Which PDF engine handles a page URI.
enum class PdfBackend {
  Default,  ///< Resolves to MuPDF when built with it
  Poppler,  ///< Deprecated alias of Default (kept for ABI)
  MuPDF,
};

/// Effective backend after applying Default + build flags.
[[nodiscard]] PdfBackend pdf_resolve_backend(PdfBackend requested);

[[nodiscard]] bool pdf_backend_available(PdfBackend backend);

[[nodiscard]] const char* pdf_backend_name(PdfBackend backend);

/**
 * Process-wide: when true, MuPDF uses Mitchell filtering for image scales
 * (including upscales of scan XObjects into denser tiles). When false,
 * MuPDF default (Mitchell on downscale only → nearest-looking upscales).
 * Default true. Hosts (biltoo View → Smooth Scaling) should mirror this.
 * Page-level TLS caches key on the flag so a toggle does not reuse stale
 * nearest-neighbour full-page buffers.
 */
void set_smooth_image_scaling(bool on);
[[nodiscard]] bool smooth_image_scaling();

struct ParsedPdfUri {
  std::filesystem::path pdf_path;
  /// 1-based page index.
  int page = 0;
  PdfBackend backend = PdfBackend::Default;
  /// URI had //text — open with MuPDF filetype magic "txt".
  bool force_text = false;
};

/// Embedded Image XObject extract (//pdfimage:N) — native pixel size, not a page render.
struct ParsedPdfImageUri {
  std::filesystem::path pdf_path;
  /// 1-based index in document order (page 1 resources, then page 2, …).
  int image = 0;
};

/// file:///abs/doc.pdf//page:12  (default backend).
[[nodiscard]] std::string pdf_page_uri(const std::filesystem::path& pdf_path,
                                       int page_1based,
                                       PdfBackend backend = PdfBackend::Default);

[[nodiscard]] std::optional<ParsedPdfUri> parse_pdf_uri(std::string_view uri);

/// file:///abs/doc.pdf//pdfimage:3 — MuPDF embedded image extract at native resolution.
[[nodiscard]] std::string pdf_image_uri(const std::filesystem::path& pdf_path,
                                        int image_1based);

[[nodiscard]] std::optional<ParsedPdfImageUri> parse_pdf_image_uri(std::string_view uri);

/// Count of extractable Image XObjects (page order, including duplicates across pages).
[[nodiscard]] std::optional<int> pdf_embedded_image_count(const std::filesystem::path& path);

[[nodiscard]] bool is_likely_pdf_path(const std::filesystem::path& path);

/// Page count, or nullopt if the file cannot be opened / is not a PDF.
[[nodiscard]] std::optional<int> pdf_page_count(
    const std::filesystem::path& path, PdfBackend backend = PdfBackend::Default);

struct PdfRaster {
  int width = 0;
  int height = 0;
  /// Contiguous RGB888 rows (no padding).
  std::vector<std::uint8_t> rgb;
};

/// Native-size RGB888 raster of the N-th embedded image (1-based). Optional
/// max_edge downscales the long edge when > 0 (ladder / LQIP).
[[nodiscard]] std::optional<PdfRaster> pdf_rasterize_embedded_image(
    const std::filesystem::path& path, int image_1based, int max_edge = 0);

[[nodiscard]] std::optional<Size> pdf_embedded_image_size(
    const std::filesystem::path& path, int image_1based);

/// What a page draws, as observed by running its display list (contents,
/// annotations and widgets) through a profiling device. Invisible text
/// (render mode 3, e.g. an OCR layer) is not visible content.
enum class PageContentKind {
  Empty,   ///< nothing visible
  Vector,  ///< visible paths / shadings / glyphs, no images
  Raster,  ///< images only — resolution is bounded by the images
  Mixed,   ///< images plus visible vector content or glyphs
};

[[nodiscard]] const char* page_content_kind_name(PageContentKind kind);

/// One image draw on a page (fill_image, fill_image_mask or clip_image_mask).
struct PdfPageImage {
  int width = 0;
  int height = 0;
  int components = 0;  ///< 1 gray / stencil, 3 RGB, 4 CMYK
  int bpc = 0;
  bool stencil = false;  ///< ImageMask (1-bit stencil painted with a colour)
  /// Pixels per inch on the page along the denser image axis.
  double dpi = 0.0;
  /// Fraction of the page covered by the image's bounding box (clipped).
  double page_fraction = 0.0;
};

struct PdfPageProfile {
  PageContentKind kind = PageContentKind::Empty;
  double width_pt = 0.0;
  double height_pt = 0.0;
  int image_draws = 0;
  /// First kPdfProfileMaxImages draws, document order.
  std::vector<PdfPageImage> images;
  /// Sum of image bbox fractions, capped at 1.
  double image_coverage = 0.0;
  int vector_paths = 0;  ///< visible fills/strokes (excluding a background fill)
  int shadings = 0;
  double vector_coverage = 0.0;  ///< sum of path/shading bbox fractions, capped at 1
  int visible_glyphs = 0;
  int clip_glyphs = 0;       ///< glyphs used as a clip (render modes 4-7)
  int invisible_glyphs = 0;  ///< render mode 3 (OCR layers)
  bool background_fill_ignored = false;
  /// Sharpest image dpi (0 when the page has no images).
  double native_dpi = 0.0;
  /// Raster pages only: coarsest scale whose dpi reaches
  /// native_dpi / kPdfNativeDpiTolerance (never finer-than-needed; ≤ 0).
  /// Finer scales add no information and are answered Unavailable.
  std::optional<int> finest_useful_scale;
  /// One line explaining the classification and the cap.
  std::string summary;
};

inline constexpr int kPdfProfileMaxImages = 32;

/// Profile of one page (built together with its display list, cached).
[[nodiscard]] std::optional<PdfPageProfile> pdf_page_profile(
    const std::filesystem::path& path, int page_1based,
    std::string* error = nullptr);

/// Why tiles at @p scale are refused for this page, or nullopt when the scale
/// is renderable. Only Raster pages refuse: scales finer than
/// PdfPageProfile::finest_useful_scale.
[[nodiscard]] std::optional<std::string> pdf_scale_refusal(
    const std::filesystem::path& path, int page_1based, int scale);

/// Image decode accounting (exact: counted inside the image decode call).
struct PdfDecodeStats {
  std::int64_t decodes = 0;           ///< actual decompressions
  std::int64_t full_decodes = 0;      ///< whole image (reused by every cell)
  std::int64_t subarea_decodes = 0;   ///< partial (image above the budget)
  std::int64_t decoded_pixels = 0;
  double decode_ms = 0.0;
  /// Cells that needed an image another thread was already decoding and
  /// waited for that decode instead of repeating it.
  std::int64_t shared_waits = 0;
  /// Largest single decode (pixels) and why it was partial, if it was.
  std::int64_t largest_decode_pixels = 0;
  std::string last_subarea_reason;
};

struct PdfPageRenderStats {
  int page = 0;  ///< 1-based
  std::int64_t display_list_builds = 0;
  double display_list_ms = 0.0;
  std::int64_t cells_rendered = 0;
  std::int64_t cells_refused = 0;
  std::int64_t cells_failed = 0;
  double render_ms = 0.0;
  std::int64_t page_rasters = 0;  ///< whole-page ladder renders
  PdfDecodeStats decode;
  std::optional<PdfPageProfile> profile;
  std::string last_error;
};

struct PdfDocumentRenderStats {
  std::string path;
  std::int64_t opens = 0;
  std::int64_t lock_waits = 0;      ///< document accesses that had to wait
  double lock_wait_ms = 0.0;
  double lock_wait_max_ms = 0.0;
  PdfDecodeStats decode;            ///< sum over pages
  std::vector<PdfPageRenderStats> pages;  ///< pages touched, ascending
};

/// Process-wide render/decode accounting for one document (cheap; any
/// thread). nullopt when the document was never opened by the PDF backend.
[[nodiscard]] std::optional<PdfDocumentRenderStats> pdf_document_render_stats(
    const std::filesystem::path& path);

void pdf_reset_render_stats();

/// Images whose decoded size fits this budget are decoded whole once and
/// shared by every cell; larger ones are decoded per cell (subarea). Default
/// kPdfFullImageDecodeBudget. Process-wide; affects decodes started later.
void pdf_set_full_image_decode_budget(std::size_t bytes);

/// Close every cached PDF document (display lists, profiles, decoded images
/// they reference). Documents in use stay alive until their users finish.
/// For memory pressure and for leak-checked shutdown.
void pdf_release_document_cache();
[[nodiscard]] std::size_t pdf_full_image_decode_budget();

/**
 * Rasterize one page so the long edge is about max_edge pixels (at least the
 * natural 72 dpi size when max_edge is large). Page is 1-based.
 */
[[nodiscard]] std::optional<PdfRaster> pdf_rasterize_page(
    const std::filesystem::path& path, int page_1based, int max_edge,
    PdfBackend backend = PdfBackend::Default);

/// Intrinsic size at 72 dpi (media box), page 1-based.
[[nodiscard]] std::optional<Size> pdf_page_size_72dpi(
    const std::filesystem::path& path, int page_1based,
    PdfBackend backend = PdfBackend::Default);

/// Layout size for Galapix-style tiles: media box scaled to kPdfLayoutDpi.
[[nodiscard]] std::optional<Size> pdf_page_layout_size(
    const std::filesystem::path& path, int page_1based,
    PdfBackend backend = PdfBackend::Default);

/**
 * Effective page pixel size at tile scale s relative to layout (kPdfLayoutDpi).
 * s=0 → layout size; s>0 → coarser (÷2 each step); s<0 → denser (×2 each step).
 */
[[nodiscard]] Size pdf_page_size_at_scale(Size layout, int scale);

/**
 * DPI for tile scale s: kPdfLayoutDpi * 2^{-s}.
 * Unclamped — region render only ever materializes ≤kTileSize² pixels.
 */
[[nodiscard]] double pdf_dpi_for_scale(int scale);

/**
 * Rasterize a pixel rectangle of the page at the given DPI.
 * (px,py,pw,ph) are in the full-page pixel grid at that DPI.
 * Page is 1-based. Does not allocate a full-page buffer beyond the crop.
 */
[[nodiscard]] std::optional<PdfRaster> pdf_rasterize_page_region(
    const std::filesystem::path& path, int page_1based, double dpi, int px,
    int py, int pw, int ph, PdfBackend backend = PdfBackend::Default);

/// Outcome of one live cell render: pixels, or a status with the reason.
struct PdfCellRender {
  TileStatus status = TileStatus::Failed;
  std::optional<PdfRaster> raster;
  std::string error;
};

/**
 * Rasterize one tile cell to RGB888 (no encode): the page display list is
 * run with the cell as the device rect, at any scale. Raster pages answer
 * Unavailable below finest_useful_scale.
 */
[[nodiscard]] PdfCellRender pdf_render_tile_cell(
    const std::filesystem::path& path, int page_1based, int scale, int x,
    int y, PdfBackend backend = PdfBackend::Default);

/**
 * Rasterize cell then JPEG-encode (durable cache only). Prefer
 * pdf_render_tile_cell + reply rgb888 for interactive live tiles.
 */
[[nodiscard]] std::optional<TileBlob> pdf_build_tile_cell(
    const std::filesystem::path& path, int page_1based, int scale, int x,
    int y, int jpeg_quality = kDefaultTileQuality,
    PdfBackend backend = PdfBackend::Default);

/**
 * Text and link regions for one page.
 * Requires MuPDF at build time.
 * Bboxes are page-space points (media box); Y up. Scale by dpi/72 to map to
 * raster pixels at a given DPI (layout uses kPdfLayoutDpi).
 */
[[nodiscard]] std::optional<PageTextLayer> pdf_page_text_layer(
    const std::filesystem::path& path, int page_1based,
    PdfBackend backend = PdfBackend::Default);

/// Flattened document outline. MuPDF only for now.
[[nodiscard]] std::optional<DocumentOutline> pdf_document_outline(
    const std::filesystem::path& path,
    PdfBackend backend = PdfBackend::Default);

/// Page /Thumb embedded preview RGB when the PDF provides it (cheap).
[[nodiscard]] std::optional<PdfRaster> pdf_page_thumb_rgb(
    const std::filesystem::path& path, int page_1based,
    PdfBackend backend = PdfBackend::Default);

}  // namespace thumtoo
