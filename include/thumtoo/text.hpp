// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace thumtoo {

/**
 * Axis-aligned rectangle in **page space**.
 *
 * Page-space conventions (normative — see docs/PAGE_SPACE.md):
 * - **PDF / EPUB (MuPDF):** origin top-left, Y down (`page_y_up == false`);
 *   units are MuPDF page points / layout box.
 * - **DjVu:** origin lower-left, Y up (`page_y_up == true`); page pixels.
 * - **Plain images / archive members (OCR):** origin top-left, Y down;
 *   units = source pixels.
 *
 * Raster (source) space is always top-left, Y-down. Hosts map with the layer's
 * `page_y_up` — never guess from path alone when a layer is present.
 */
struct TextRect {
  double x0 = 0;
  double y0 = 0;
  double x1 = 0;
  double y1 = 0;

  [[nodiscard]] double width() const { return x1 - x0; }
  [[nodiscard]] double height() const { return y1 - y0; }
  [[nodiscard]] bool empty() const { return width() <= 0 || height() <= 0; }
};

enum class TextRegionRole : std::uint8_t {
  Text = 0,
  Link = 1,
};

/// Semantic label (OCR post-pass / future LLM). Independent of Role.
enum class TextRegionKind : std::uint8_t {
  Body = 0,
  PageNumber = 1,
  Header = 2,
  Footer = 3,
};

enum class TextLinkTargetKind : std::uint8_t {
  None = 0,
  InternalPage = 1,  ///< 1-based page index
  Uri = 2,
};

struct TextLinkTarget {
  TextLinkTargetKind kind = TextLinkTargetKind::None;
  int page_1based = 0;   ///< InternalPage
  double x = 0;          ///< optional dest point (page space)
  double y = 0;
  std::string uri;       ///< Uri
};

/// One selectable / searchable / clickable region on a page.
struct TextRegion {
  TextRect bbox;
  TextRegionRole role = TextRegionRole::Text;
  std::string text;  ///< role=Text: content; role=Link: optional label
  TextLinkTarget target;
  /// MuPDF structured-text block index (0-based) when known; -1 otherwise.
  int block_id = -1;
  TextRegionKind kind = TextRegionKind::Body;
};

/// Provenance of a page text layer (native extract vs OCR backend).
enum class TextLayerSource : std::uint8_t {
  Native = 0,
  Ocr = 1,
};

/// Backend metadata when source == Ocr. Engine-agnostic so LLM OCR can reuse.
struct OcrMeta {
  std::string engine;
  std::string engine_version;
  std::string model;
  std::string lang;
  int dpi = 0;
  std::int64_t created_unix = 0;
  std::vector<std::pair<std::string, std::string>> params;
};

struct PageTextLayer {
  int page_1based = 0;
  std::string layout_key;
  TextRect page_bounds;
  /**
   * When true, region bboxes use bottom-left origin (Y up) inside page_bounds.
   * When false, top-left origin (Y down). See TextRect docs.
   * Always set by extractors / OCR; hosts must honour this flag.
   */
  bool page_y_up = true;
  std::vector<TextRegion> regions;
  TextLayerSource source = TextLayerSource::Native;
  std::optional<OcrMeta> ocr;
};

struct OutlineItem {
  int level = 1;
  std::string title;
  int page_1based = 0;
  std::string uri;
};

struct DocumentOutline {
  std::vector<OutlineItem> items;
};

[[nodiscard]] std::string ocr_store_layout_key(std::string_view base_layout_key,
                                               std::string_view engine,
                                               std::string_view model);

[[nodiscard]] std::vector<std::uint8_t> serialize_page_text_layer(
    const PageTextLayer& layer);

[[nodiscard]] std::optional<PageTextLayer> deserialize_page_text_layer(
    const std::vector<std::uint8_t>& bytes);

[[nodiscard]] std::vector<std::uint8_t> serialize_document_outline(
    const DocumentOutline& outline);

[[nodiscard]] std::optional<DocumentOutline> deserialize_document_outline(
    const std::vector<std::uint8_t>& bytes);

[[nodiscard]] std::optional<PageTextLayer> extract_page_text_layer(
    std::string_view uri);

[[nodiscard]] std::optional<DocumentOutline> extract_document_outline(
    std::string_view uri);

}  // namespace thumtoo
