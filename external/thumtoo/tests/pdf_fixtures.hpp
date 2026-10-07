// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Synthetic PDF pages covering the page-content classes thumtoo tells apart
// (PageContentKind) — built with MuPDF's PDF writer, so tests need no
// external tools. benchtoo's generators/gen_pdf_classes.py builds the same
// classes with reportlab for corpus-level checks.

#include "thumtoo/pdf.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace thumtoo::fixtures {

struct PdfFixture {
  std::string name;
  PageContentKind kind;
  double native_dpi = 0.0;  ///< expected sharpest image dpi (0 = no images)
  std::optional<int> finest_useful_scale;
  bool background_fill_ignored = false;
  bool invisible_text = false;
};

/// All fixtures in the order write_fixture_pdf() emits them (one per page).
[[nodiscard]] std::vector<PdfFixture> pdf_fixtures();

/// One PDF with every fixture as a page (page i+1 = pdf_fixtures()[i]).
/// Small pages (2 x 2 in) keep images cheap; dpi values are still real.
/// Returns an error message, or empty on success.
[[nodiscard]] std::string write_fixture_pdf(const std::filesystem::path& out);

/// A Letter page of vector text and lines (no images) for deep-zoom tests.
[[nodiscard]] std::string write_vector_letter_pdf(const std::filesystem::path& out);

}  // namespace thumtoo::fixtures
