// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/text.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace thumtoo {

struct OcrOptions {
  /// Tesseract language codes (e.g. "eng", "eng+deu"). Empty → "eng".
  std::string lang;
  /// Target raster long-edge pixels (clamped). 0 → default (3000).
  int max_edge = 0;
  /**
   * Source resolution told to Tesseract (dots per inch). 0 → estimate from the
   * raster vs page_bounds (PDF points) or a 300 DPI default for pixel page
   * boxes. Wrong DPI makes mixed font sizes segment poorly — always set when
   * the host knows the true scale (e.g. appearance OCR of a cropped page).
   */
  int dpi = 0;
  /// Engine id for provenance / store key. Empty → "tesseract".
  std::string engine;
  /// Model / tessdata name. Empty → "default".
  std::string model;
  /**
   * Optional crop in the same page space as PageTextLayer::page_bounds /
   * region bboxes. When set (has_crop && crop width/height > 0), the OCR
   * raster is cropped before Tesseract so content outside the crop is not
   * recognized. Result region bboxes remain in full page space.
   */
  bool has_crop = false;
  double crop_x0 = 0;
  double crop_y0 = 0;
  double crop_x1 = 0;
  double crop_y1 = 0;
};

/// True when built with THUMTOO_HAVE_TESSERACT and runtime init succeeds once.
[[nodiscard]] bool ocr_available();

/// Thread-local reason for the last failed ocr_page_text_layer / Init (empty if ok).
[[nodiscard]] std::string_view ocr_last_error();

/**
 * Rasterize @p uri (PDF/DjVu page, archive member, or image file) and run OCR →
 * PageTextLayer with source=Ocr. Does not touch Store. Returns nullopt if OCR is
 * unavailable or the locator cannot be rasterized; see ocr_last_error().
 */
[[nodiscard]] std::optional<PageTextLayer> ocr_page_text_layer(
    std::string_view uri, const OcrOptions& opts = {});

/**
 * Run OCR on a caller-provided RGB888 buffer (row-major, 3 bytes/pixel).
 * @p page_bounds defines the coordinate system for region bboxes (same as
 * PageTextLayer). Boxes are top-left Y-down in that box (`page_y_up = false`);
 * the host remaps to document page space when needed. Does not touch Store.
 * Used when the host has already applied crop / orient / colour grade so OCR
 * matches on-screen pixels.
 *
 * Prefer setting @p opts.dpi to the true raster density (e.g. 72 × native_w /
 * page_bounds_width_in_points) so Tesseract segments mixed type sizes correctly.
 */
[[nodiscard]] std::optional<PageTextLayer> ocr_rgb_page_text_layer(
    const std::uint8_t* rgb, int width, int height,
    const TextRect& page_bounds, const OcrOptions& opts = {});

}  // namespace thumtoo
