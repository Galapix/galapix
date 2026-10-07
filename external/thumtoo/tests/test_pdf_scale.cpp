// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/constants.hpp"
#include "thumtoo/pdf.hpp"

#include <cmath>
#include <iostream>

namespace {

int g_failures = 0;

void expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << "\n";
    ++g_failures;
  }
}

void expect_eq(int a, int b, const char* msg) {
  if (a != b) {
    std::cerr << "FAIL: " << msg << " (" << a << " != " << b << ")\n";
    ++g_failures;
  }
}

void expect_near(double a, double b, double tol, const char* msg) {
  if (std::abs(a - b) > tol) {
    std::cerr << "FAIL: " << msg << " (" << a << " vs " << b << ")\n";
    ++g_failures;
  }
}

}  // namespace

int main() {
  // Layout at 144 dpi for a 612×792 pt letter page (media box).
  const thumtoo::Size s72{612, 792};
  const double layout_scale =
      static_cast<double>(thumtoo::kPdfLayoutDpi) / 72.0;
  expect_eq(thumtoo::kPdfLayoutDpi, 144, "layout dpi is 144 (do not change lightly)");
  const thumtoo::Size layout{
      static_cast<int>(std::lround(s72.width * layout_scale)),
      static_cast<int>(std::lround(s72.height * layout_scale))};
  expect_eq(layout.width, 1224, "letter layout width @144dpi");
  expect_eq(layout.height, 1584, "letter layout height @144dpi");

  // scale 0 = layout
  auto f0 = thumtoo::pdf_page_size_at_scale(layout, 0);
  expect_eq(f0.width, layout.width, "scale0 width");
  expect_eq(f0.height, layout.height, "scale0 height");
  expect_near(thumtoo::pdf_dpi_for_scale(0), 144.0, 1e-9, "dpi scale0");

  // scale +1 = half
  auto f1 = thumtoo::pdf_page_size_at_scale(layout, 1);
  expect_eq(f1.width, 612, "scale+1 width");
  expect_eq(f1.height, 792, "scale+1 height");
  expect_near(thumtoo::pdf_dpi_for_scale(1), 72.0, 1e-9, "dpi scale+1");

  // scale -1 = 2× layout pixels, 288 dpi
  auto fm1 = thumtoo::pdf_page_size_at_scale(layout, -1);
  expect_eq(fm1.width, 2448, "scale-1 width");
  expect_eq(fm1.height, 3168, "scale-1 height");
  expect_near(thumtoo::pdf_dpi_for_scale(-1), 288.0, 1e-9, "dpi scale-1");

  // Tile (0,0) at scale -1 covers 256×256 of the 2L grid = 128×128 of layout.
  // Galapix places it with scale_factor=0.5 → world size 128 layout px. Match.
  const int T = thumtoo::kTileSize;
  expect_eq(T, 256, "tile size");
  const int layout_span_s0 = T;             // scale 0
  const int layout_span_sm1 = T / 2;        // scale -1
  expect_eq(layout_span_sm1, 128, "scale-1 covers 128 layout px per full tile");
  // Number of tiles along width at scale -1:
  const int tiles_w = (fm1.width + T - 1) / T;
  expect_eq(tiles_w, (layout.width * 2 + T - 1) / T, "tile count scale-1");

  // scale -2
  expect_near(thumtoo::pdf_dpi_for_scale(-2), 576.0, 1e-9, "dpi scale-2");
  auto fm2 = thumtoo::pdf_page_size_at_scale(layout, -2);
  expect_eq(fm2.width, layout.width * 4, "scale-2 width");

  if (g_failures) {
    std::cerr << "test_pdf_scale: " << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "test_pdf_scale: ok\n";
  return 0;
}
