// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/djvu.hpp"
#include "thumtoo/text.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

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
#if !defined(THUMTOO_HAVE_DJVU)
  std::cout << "SKIP: DjVu not available at build time\n";
  return 0;
#else
  // Optional fixture path via env (no durable sample in tree yet).
  const char* fixture = std::getenv("THUMTOO_TEST_DJVU");
  if (!fixture || !*fixture) {
    std::cout << "SKIP: set THUMTOO_TEST_DJVU to a .djvu with a text layer\n";
    // API must still be linkable and reject bad paths cleanly.
    auto missing = thumtoo::djvu_page_text_layer("/nonexistent/nope.djvu", 1);
    expect(!missing.has_value(), "missing file → nullopt");
    auto out = thumtoo::djvu_document_outline("/nonexistent/nope.djvu");
    expect(!out.has_value(), "missing outline → nullopt");
    if (g_failures) return 1;
    std::cout << "OK test_djvu_text (smoke)\n";
    return 0;
  }

  const std::filesystem::path path(fixture);
  if (!std::filesystem::exists(path)) {
    std::cerr << "FAIL: fixture does not exist: " << path << "\n";
    return 1;
  }

  auto layer = thumtoo::djvu_page_text_layer(path, 1);
  expect(layer.has_value(), "text layer call succeeds");
  if (layer) {
    expect(layer->page_1based == 1, "page index");
    expect(layer->page_bounds.width() > 0, "page width");
    // Text may be empty if the file has no OCR layer — that is valid.
    for (const auto& r : layer->regions) {
      if (r.role == thumtoo::TextRegionRole::Text) {
        expect(!r.text.empty(), "text region non-empty string");
        expect(!r.bbox.empty(), "text region bbox");
      } else if (r.role == thumtoo::TextRegionRole::Link) {
        expect(r.target.kind != thumtoo::TextLinkTargetKind::None, "link target");
      }
    }
  }

  auto outline = thumtoo::djvu_document_outline(path);
  expect(outline.has_value(), "outline call succeeds");

  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "OK test_djvu_text\n";
  return 0;
#endif
}
