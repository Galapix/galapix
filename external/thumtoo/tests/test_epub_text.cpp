// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/epub.hpp"
#include "thumtoo/text.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>

namespace {

int g_failures = 0;

void expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << "\n";
    ++g_failures;
  }
}

}  // namespace

int main() {
  if (!thumtoo::epub_available()) {
    std::cout << "SKIP: EPUB/MuPDF not available\n";
    return 0;
  }

  const char* fixture = std::getenv("THUMTOO_TEST_EPUB");
  if (!fixture || !*fixture) {
    auto missing = thumtoo::epub_page_text_layer("/nonexistent/nope.epub", 1);
    expect(!missing.has_value(), "missing file → nullopt");
    auto out = thumtoo::epub_document_outline("/nonexistent/nope.epub");
    expect(!out.has_value(), "missing outline → nullopt");
    if (g_failures) return 1;
    std::cout << "OK test_epub_text (smoke)\n";
    std::cout << "  set THUMTOO_TEST_EPUB to exercise a real book\n";
    return 0;
  }

  const std::filesystem::path path(fixture);
  if (!std::filesystem::exists(path)) {
    std::cerr << "FAIL: fixture missing: " << path << "\n";
    return 1;
  }

  const auto layout = thumtoo::default_epub_layout();
  auto layer = thumtoo::epub_page_text_layer(path, 1, layout);
  expect(layer.has_value(), "text layer extracted");
  if (layer) {
    expect(layer->page_1based == 1, "page index");
    expect(!layer->layout_key.empty(), "layout_key set");
    expect(layer->page_bounds.width() > 0, "page bounds");
    // Real books almost always have some text on page 1.
    int text_n = 0;
    for (const auto& r : layer->regions) {
      if (r.role == thumtoo::TextRegionRole::Text && !r.text.empty()) ++text_n;
    }
    // Do not hard-fail if cover is image-only.
    (void)text_n;
  }

  auto outline = thumtoo::epub_document_outline(path, layout);
  expect(outline.has_value(), "outline call succeeds");

  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "OK test_epub_text\n";
  return 0;
}
