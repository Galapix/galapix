// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// PDF page profile, resolution cap, decode-once rendering and seam-free cells.
///
/// * Every fixture class (pdf_fixtures.hpp) gets the expected PageContentKind,
///   native dpi and finest useful scale; invisible OCR text and page-size
///   background fills do not count as vector detail.
/// * Rendering every cell of a page at one scale decodes each image once,
///   also with several threads (shared store, shared display list).
/// * Stitched cells equal one region render of the same area (no seams).
/// * Vector pages render far beyond the old 100 MP full-page limit.
/// * Raster pages answer Unavailable below their cap, with the reason.

#include "pdf_fixtures.hpp"

#include "thumtoo/pdf.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/image.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using thumtoo::PageContentKind;
using thumtoo::TileStatus;

namespace {

int g_failures = 0;

void expect(bool ok, const std::string& msg) {
  if (!ok) {
    std::cerr << "FAIL: " << msg << "\n";
    ++g_failures;
  }
}

const thumtoo::PdfPageRenderStats* page_stats(const thumtoo::PdfDocumentRenderStats& d,
                                               int page) {
  for (const auto& p : d.pages) {
    if (p.page == page) return &p;
  }
  return nullptr;
}

std::int64_t decodes(const fs::path& pdf, int page) {
  auto st = thumtoo::pdf_document_render_stats(pdf);
  if (!st) return -1;
  const auto* p = page_stats(*st, page);
  return p ? p->decode.decodes : 0;
}

/// All cells of @p page at @p scale stitched into one RGB buffer.
std::vector<std::uint8_t> stitch(const fs::path& pdf, int page, int scale, int w, int h) {
  std::vector<std::uint8_t> out(static_cast<std::size_t>(w) * h * 3, 0);
  const int nx = (w + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
  const int ny = (h + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
  for (int ty = 0; ty < ny; ++ty) {
    for (int tx = 0; tx < nx; ++tx) {
      auto cell = thumtoo::pdf_render_tile_cell(pdf, page, scale, tx, ty);
      if (!cell.raster) {
        expect(false, "stitch cell " + std::to_string(tx) + "," + std::to_string(ty) +
                          ": " + cell.error);
        continue;
      }
      const auto& r = *cell.raster;
      for (int y = 0; y < r.height; ++y) {
        std::copy_n(&r.rgb[static_cast<std::size_t>(y) * r.width * 3], r.width * 3,
                    &out[(static_cast<std::size_t>(ty * thumtoo::kTileSize + y) * w +
                          tx * thumtoo::kTileSize) *
                         3]);
      }
    }
  }
  return out;
}

/// @p max_level: largest allowed per-byte difference. 0 = byte-exact. MuPDF's
/// stroker clips segments to the scissor in floating point
/// (fitz/draw-path.c, do_flatten_stroke s.rect), so diagonal strokes differ
/// by a few levels on anti-aliased edge pixels between any two clip regions.
void check_seams(const fs::path& pdf, int page, int scale, const std::string& what,
                 int max_level = 0) {
  auto layout = thumtoo::pdf_page_layout_size(pdf, page);
  if (!layout) {
    expect(false, what + ": layout size");
    return;
  }
  const auto full = thumtoo::pdf_page_size_at_scale(*layout, scale);
  const auto stitched = stitch(pdf, page, scale, full.width, full.height);
  auto whole = thumtoo::pdf_rasterize_page_region(
      pdf, page, thumtoo::pdf_dpi_for_scale(scale), 0, 0, full.width, full.height);
  expect(static_cast<bool>(whole), what + ": whole-page region render");
  if (!whole) return;
  std::size_t diff = 0;
  int maxd = 0;
  for (std::size_t i = 0; i < stitched.size(); ++i) {
    const int d = std::abs(int(stitched[i]) - int(whole->rgb[i]));
    if (d) ++diff;
    maxd = std::max(maxd, d);
  }
  expect(maxd <= max_level, what + ": stitched cells differ from one render in " +
                        std::to_string(diff) + " bytes (max " + std::to_string(maxd) +
                        ")");
}

}  // namespace

int main() {
  const fs::path dir = fs::temp_directory_path() /
                       ("thumtoo-pdf-profile-" + std::to_string(std::rand()));
  fs::create_directories(dir);
  const fs::path pdf = dir / "classes.pdf";
  const fs::path letter = dir / "letter.pdf";
  {
    const std::string e1 = thumtoo::fixtures::write_fixture_pdf(pdf);
    const std::string e2 = thumtoo::fixtures::write_vector_letter_pdf(letter);
    expect(e1.empty(), "write fixtures: " + e1);
    expect(e2.empty(), "write letter: " + e2);
    if (!e1.empty() || !e2.empty()) return 1;
  }

  // 1. Classification.
  const auto fixtures = thumtoo::fixtures::pdf_fixtures();
  expect(thumtoo::pdf_page_count(pdf).value_or(0) == static_cast<int>(fixtures.size()),
         "fixture page count");
  for (std::size_t i = 0; i < fixtures.size(); ++i) {
    const auto& fx = fixtures[i];
    const int page = static_cast<int>(i) + 1;
    std::string error;
    auto prof = thumtoo::pdf_page_profile(pdf, page, &error);
    expect(static_cast<bool>(prof), fx.name + ": profile (" + error + ")");
    if (!prof) continue;
    const std::string ctx = fx.name + " [" + prof->summary + "]";
    expect(prof->kind == fx.kind, ctx + ": kind " +
                                      thumtoo::page_content_kind_name(prof->kind) +
                                      ", want " + thumtoo::page_content_kind_name(fx.kind));
    expect(std::abs(prof->native_dpi - fx.native_dpi) < 1.0,
           ctx + ": native dpi " + std::to_string(prof->native_dpi));
    expect(prof->finest_useful_scale == fx.finest_useful_scale,
           ctx + ": finest useful scale " +
               (prof->finest_useful_scale ? std::to_string(*prof->finest_useful_scale)
                                          : std::string("none")));
    expect(prof->background_fill_ignored == fx.background_fill_ignored,
           ctx + ": background fill");
    expect((prof->invisible_glyphs > 0) == fx.invisible_text, ctx + ": invisible glyphs");
    expect(!prof->summary.empty(), ctx + ": summary");
  }

  // 2. Decode once per (image, subsample level): every cell of the 600 dpi
  //    page at scale -2 shares one full decode.
  thumtoo::pdf_reset_render_stats();
  const int rgb600 = 5;
  {
    const auto layout = *thumtoo::pdf_page_layout_size(pdf, rgb600);
    const auto full = thumtoo::pdf_page_size_at_scale(layout, -2);
    const int nx = (full.width + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
    const int ny = (full.height + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
    int ok = 0;
    for (int y = 0; y < ny; ++y) {
      for (int x = 0; x < nx; ++x) {
        auto cell = thumtoo::pdf_render_tile_cell(pdf, rgb600, -2, x, y);
        ok += cell.status == TileStatus::Ok;
      }
    }
    expect(ok == nx * ny, "all cells at -2 render");
    expect(nx * ny > 1, "page spans several cells");
    expect(decodes(pdf, rgb600) == 1, "one decode for " + std::to_string(nx * ny) +
                                          " cells, got " +
                                          std::to_string(decodes(pdf, rgb600)));
    auto st = thumtoo::pdf_document_render_stats(pdf);
    const auto* p = st ? page_stats(*st, rgb600) : nullptr;
    expect(p && p->decode.full_decodes == 1, "the decode was a full decode");
    expect(p && p->cells_rendered == nx * ny, "cells_rendered counted");
    expect(p && p->display_list_builds <= 1, "display list built at most once");
  }

  // 3. Same with threads: shared store and display list → still ~one decode
  //    (two racing threads may both decode before either stores).
  thumtoo::pdf_reset_render_stats();
  {
    const int scan = 4;  // scan-gray-300
    const auto layout = *thumtoo::pdf_page_layout_size(pdf, scan);
    const auto full = thumtoo::pdf_page_size_at_scale(layout, -1);
    const int nx = (full.width + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
    const int ny = (full.height + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
    constexpr int kThreads = 4;
    std::atomic<int> ok{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < kThreads; ++t) {
      threads.emplace_back([&] {
        for (int y = 0; y < ny; ++y) {
          for (int x = 0; x < nx; ++x) {
            ok += thumtoo::pdf_render_tile_cell(pdf, scan, -1, x, y).status ==
                  TileStatus::Ok;
          }
        }
      });
    }
    for (auto& t : threads) t.join();
    expect(ok == kThreads * nx * ny, "threaded cells render");
    const auto n = decodes(pdf, scan);
    expect(n == 1, "threaded decodes " + std::to_string(n) +
                       " (concurrent cells wait for the one decode)");
  }

  // 4. No seams: stitched cells == one region render.
  check_seams(pdf, 4, -1, "scan-gray-300 at -1");
  check_seams(pdf, 9, -1, "mixed-raster at -1");
  check_seams(pdf, 3, -1, "vector-diagram at -1", 24);
  check_seams(letter, 1, 0, "letter at 0");

  // 4b. Over the full-decode budget: per-cell subarea decodes are counted
  //     with a reason. (They are not seam-free: MuPDF re-derives the image
  //     matrix per subarea, so each cell resamples with a slightly different
  //     phase — the reason whole-image decode is the default.)
  thumtoo::pdf_reset_render_stats();
  {
    const std::size_t saved = thumtoo::pdf_full_image_decode_budget();
    thumtoo::pdf_set_full_image_decode_budget(0);
    // Page 7 (scan-white-background) was never rendered: nothing in the store.
    const int page = 7;
    const auto layout = *thumtoo::pdf_page_layout_size(pdf, page);
    const auto full = thumtoo::pdf_page_size_at_scale(layout, -1);
    stitch(pdf, page, -1, full.width, full.height);
    thumtoo::pdf_set_full_image_decode_budget(saved);
    auto st = thumtoo::pdf_document_render_stats(pdf);
    const auto* p = st ? page_stats(*st, page) : nullptr;
    expect(p && p->decode.subarea_decodes > 1,
           "subarea decodes counted: " +
               std::to_string(p ? p->decode.subarea_decodes : -1));
    expect(p && p->decode.full_decodes == 0, "no full decode over budget");
    expect(p && !p->decode.last_subarea_reason.empty(), "subarea reason recorded");
  }

  // 5. Vector pages: no full-page budget. Letter at -4 is 19584x25344 (496 MP).
  {
    auto cell = thumtoo::pdf_render_tile_cell(letter, 1, -4, 40, 50);
    expect(cell.status == TileStatus::Ok,
           std::string("letter -4 cell: ") + thumtoo::tile_status_name(cell.status) +
               " " + cell.error);
    expect(cell.raster && cell.raster->width == thumtoo::kTileSize,
           "letter -4 cell is a full tile");
  }

  // 6. Raster cap: below finest_useful_scale → Unavailable with the reason.
  {
    auto cell = thumtoo::pdf_render_tile_cell(pdf, 4, -2, 0, 0);  // 300 dpi, cap -1
    expect(cell.status == TileStatus::Unavailable,
           std::string("scan -2: ") + thumtoo::tile_status_name(cell.status));
    expect(cell.error.find("finest useful scale -1") != std::string::npos,
           "refusal reason: " + cell.error);
    expect(thumtoo::pdf_scale_refusal(pdf, 4, -2).has_value(), "pdf_scale_refusal -2");
    expect(!thumtoo::pdf_scale_refusal(pdf, 4, -1).has_value(), "pdf_scale_refusal -1");
    expect(!thumtoo::pdf_scale_refusal(pdf, 10, -4).has_value(),
           "mixed pages are never refused");
  }

  // 7. Errors carry a reason.
  {
    auto cell = thumtoo::pdf_render_tile_cell(dir / "missing.pdf", 1, 0, 0, 0);
    expect(cell.status == TileStatus::Failed && !cell.error.empty(),
           "missing file fails with a reason: " + cell.error);
    auto past = thumtoo::pdf_render_tile_cell(pdf, 99, 0, 0, 0);
    expect(past.status == TileStatus::Failed &&
               past.error.find("out of range") != std::string::npos,
           "page out of range: " + past.error);
  }

  thumtoo::pdf_release_document_cache();
  std::error_code ec;
  fs::remove_all(dir, ec);
  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "pdf_profile OK\n";
  return 0;
}
