// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// DjVu tile cells, page profiles and decode accounting on
/// tests/fixtures/pages.djvu (make_djvu_fixtures.sh):
///   1 bitonal 2550x3300 @300 + hidden text, 2 photo 1275x1650 @150,
///   3 compound 2550x3300 (300 dpi JB2 mask + 850x1100 IW44 background),
///   4 empty (INFO only).
///
/// * Profiles report the layers with their dpi; finest useful scale is 0.
/// * Every cell of a page renders from one page decode (also threaded, and
///   with two documents open).
/// * The coarser grid floor-halves like the host (637, not 638).
/// * Stitched cells match one region render (exact for JB2; IW44 within a few
///   levels — djvulibre reconstructs wavelets per render rect).
/// * Finer-than-native scales, missing files and bad pages answer with a
///   status and a reason.

#include "thumtoo/djvu.hpp"
#include "thumtoo/constants.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;
using thumtoo::TileStatus;

namespace {

int g_failures = 0;

void expect(bool ok, const std::string& msg) {
  if (!ok) {
    std::cerr << "FAIL: " << msg << "\n";
    ++g_failures;
  }
}

const thumtoo::PdfPageRenderStats* page_stats(const fs::path& f, int page,
                                              thumtoo::PdfDocumentRenderStats* keep) {
  auto st = thumtoo::djvu_document_render_stats(f);
  if (!st) return nullptr;
  *keep = *st;
  for (const auto& p : keep->pages) {
    if (p.page == page) return &p;
  }
  return nullptr;
}

int render_all(const fs::path& f, int page, int scale) {
  auto layout = thumtoo::djvu_page_layout_size(f, page);
  if (!layout) return -1;
  const auto full = thumtoo::djvu_page_size_at_scale(*layout, scale);
  const int nx = (full.width + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
  const int ny = (full.height + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
  int ok = 0;
  for (int y = 0; y < ny; ++y) {
    for (int x = 0; x < nx; ++x) {
      ok += thumtoo::djvu_render_tile_cell(f, page, scale, x, y).status == TileStatus::Ok;
    }
  }
  return ok == nx * ny ? ok : -ok;
}

}  // namespace

int main() {
  const fs::path f = fs::path(THUMTOO_FIXTURES_DIR) / "pages.djvu";
  expect(thumtoo::djvu_page_count(f).value_or(0) == 4, "page count");

  // 1. Profiles.
  {
    std::string error;
    auto p1 = thumtoo::djvu_page_profile(f, 1, &error);
    expect(p1 && p1->kind == thumtoo::PageContentKind::Raster, "bitonal raster " + error);
    if (p1) {
      expect(p1->native_dpi == 300, "bitonal dpi");
      expect(p1->finest_useful_scale == 0, "bitonal cap 0");
      expect(p1->images.size() == 1 && p1->images[0].stencil, "bitonal JB2 mask layer");
      expect(p1->summary.find("bitonal") != std::string::npos, "summary: " + p1->summary);
    }
    auto p2 = thumtoo::djvu_page_profile(f, 2, &error);
    expect(p2 && p2->images.size() == 1 && p2->images[0].components == 3 &&
               p2->native_dpi == 150,
           "photo: one colour IW44 layer at 150 dpi");
    auto p3 = thumtoo::djvu_page_profile(f, 3, &error);
    expect(p3 && p3->images.size() == 2, "compound: mask + background");
    if (p3 && p3->images.size() == 2) {
      expect(p3->images[0].stencil && p3->images[0].dpi == 300, "compound mask 300 dpi");
      expect(p3->images[1].width == 850 && std::abs(p3->images[1].dpi - 100.0) < 0.5,
             "compound background 850 px at 100 dpi");
      expect(p3->summary.find("compound") != std::string::npos, "summary: " + p3->summary);
    }
  }

  // 2. Decode once per page for all its cells.
  thumtoo::djvu_release_document_cache();
  thumtoo::djvu_reset_render_stats();
  {
    expect(render_all(f, 3, 0) == 130, "compound: 130 cells at scale 0");
    expect(render_all(f, 3, 1) > 0, "compound: cells at scale 1");
    thumtoo::PdfDocumentRenderStats keep;
    const auto* p = page_stats(f, 3, &keep);
    expect(p && p->decode.decodes == 1,
           "one page decode for all cells, got " +
               std::to_string(p ? p->decode.decodes : -1));
    expect(p && p->cells_rendered > 130, "cells counted");
  }

  // 3. Threads and two documents: still one decode per (document, page).
  const fs::path dir =
      fs::temp_directory_path() /
      ("thumtoo-djvu-" +
       std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
  fs::create_directories(dir);
  const fs::path g = dir / "copy.djvu";
  fs::copy_file(f, g, fs::copy_options::overwrite_existing);
  thumtoo::djvu_release_document_cache();
  thumtoo::djvu_reset_render_stats();
  {
    std::atomic<int> bad{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
      threads.emplace_back([&, t] {
        const fs::path& doc = (t % 2) ? g : f;
        for (int page = 1; page <= 3; ++page) {
          if (render_all(doc, page, 1) <= 0) ++bad;
        }
      });
    }
    for (auto& th : threads) th.join();
    expect(bad == 0, "threaded renders");
    for (const fs::path* doc : {&f, &g}) {
      for (int page = 1; page <= 3; ++page) {
        thumtoo::PdfDocumentRenderStats keep;
        const auto* p = page_stats(*doc, page, &keep);
        expect(p && p->decode.decodes == 1,
               doc->filename().string() + " page " + std::to_string(page) + " decodes " +
                   std::to_string(p ? p->decode.decodes : -1));
      }
    }
  }

  // 4. Grid geometry matches the host (floor-halving).
  {
    const auto s2 = thumtoo::djvu_page_size_at_scale({2550, 3300}, 2);
    expect(s2.width == 637 && s2.height == 825, "scale 2 is 637x825");
    auto edge = thumtoo::djvu_render_tile_cell(f, 1, 2, 2, 3);
    expect(edge.raster && edge.raster->width == 637 - 512 && edge.raster->height == 825 - 768,
           "edge cell is 125x57");
  }

  // 5. No seams: stitched scale-0 cells == one region render.
  for (int page = 1; page <= 3; ++page) {
    const auto layout = *thumtoo::djvu_page_layout_size(f, page);
    const int w = layout.width, h = layout.height;
    std::vector<std::uint8_t> stitched(static_cast<std::size_t>(w) * h * 3);
    for (int ty = 0; ty * thumtoo::kTileSize < h; ++ty) {
      for (int tx = 0; tx * thumtoo::kTileSize < w; ++tx) {
        auto c = thumtoo::djvu_render_tile_cell(f, page, 0, tx, ty);
        if (!c.raster) continue;
        for (int y = 0; y < c.raster->height; ++y) {
          std::copy_n(&c.raster->rgb[static_cast<std::size_t>(y) * c.raster->width * 3],
                      c.raster->width * 3,
                      &stitched[(static_cast<std::size_t>(ty * thumtoo::kTileSize + y) * w +
                                 tx * thumtoo::kTileSize) * 3]);
        }
      }
    }
    auto whole = thumtoo::djvu_rasterize_page_region(f, page, 1.0, 0, 0, w, h);
    std::size_t diff = 0, edge = 0;
    int maxd = 0;
    if (whole) {
      for (std::size_t i = 0; i < stitched.size(); ++i) {
        const int d = std::abs(int(stitched[i]) - int(whole->rgb[i]));
        if (!d) continue;
        ++diff;
        maxd = std::max(maxd, d);
        const int px = static_cast<int>(i / 3);
        const int bx = (px % w) % thumtoo::kTileSize, by = (px / w) % thumtoo::kTileSize;
        if (bx < 3 || bx > 252 || by < 3 || by > 252) ++edge;
      }
    }
    if (diff && std::getenv("THUMTOO_TEST_VERBOSE")) {
      std::cerr << "page " << page << ": " << diff << " bytes differ (max " << maxd << "), "
                << edge << " within 3px of a cell edge\n";
    }
    // JB2 (bitonal) is exact. djvulibre reconstructs IW44 wavelets per render
    // rect, so photo/background layers differ by a few levels between rects
    // (measured max 6; a 128 px margin per cell would be needed to remove it).
    expect(whole && maxd <= 8, "stitched cells match one render within 8 levels");
    if (page == 1) expect(diff == 0, "bitonal page stitches exactly");
  }

  // 6. Reasons.
  {
    auto fine = thumtoo::djvu_render_tile_cell(f, 1, -1, 0, 0);
    expect(fine.status == TileStatus::Unavailable &&
               fine.error.find("native pixels") != std::string::npos,
           "scale -1 Unavailable: " + fine.error);
    auto missing = thumtoo::djvu_render_tile_cell(dir / "missing.djvu", 1, 0, 0, 0);
    expect(missing.status == TileStatus::Failed && !missing.error.empty(),
           "missing file: " + missing.error);
    auto past = thumtoo::djvu_render_tile_cell(f, 9, 0, 0, 0);
    expect(past.status == TileStatus::Failed &&
               past.error.find("out of range") != std::string::npos,
           "page out of range: " + past.error);
    auto outside = thumtoo::djvu_render_tile_cell(f, 1, 0, 99, 0);
    expect(outside.status == TileStatus::Unavailable, "cell outside page: " + outside.error);
  }

  // 7. Hidden text layer and empty pages.
  {
    std::string error;
    auto p1 = thumtoo::djvu_page_profile(f, 1, &error);
    expect(p1 && p1->invisible_glyphs > 40, "hidden text counted: " +
                                                std::to_string(p1 ? p1->invisible_glyphs : -1));
    expect(p1 && p1->summary.find("hidden text") != std::string::npos,
           "summary mentions the text layer");
    auto layer = thumtoo::djvu_page_text_layer(f, 1);
    std::size_t words = 0;
    if (layer) {
      for (const auto& r : layer->regions) words += r.role == thumtoo::TextRegionRole::Text;
    }
    expect(words == 9, "nine words in the text layer, got " + std::to_string(words));

    auto empty = thumtoo::djvu_page_profile(f, 4, &error);
    expect(empty && empty->kind == thumtoo::PageContentKind::Empty && empty->images.empty(),
           "INFO-only page is Empty: " + (empty ? empty->summary : error));
    auto cell = thumtoo::djvu_render_tile_cell(f, 4, 0, 1, 1);
    bool white = cell.raster.has_value();
    if (cell.raster) {
      for (auto b : cell.raster->rgb) white = white && b == 255;
    }
    expect(cell.status == TileStatus::Ok && white, "empty page renders white: " + cell.error);
  }

  // 8. Broken files fail with a reason (not white tiles).
  {
    const fs::path truncated = dir / "truncated.djvu";
    {
      std::ifstream in(f, std::ios::binary);
      std::vector<char> bytes((std::istreambuf_iterator<char>(in)), {});
      bytes.resize(bytes.size() / 3);
      std::ofstream(truncated, std::ios::binary).write(bytes.data(), bytes.size());
    }
    auto t = thumtoo::djvu_render_tile_cell(truncated, 3, 0, 0, 0);
    expect(t.status == TileStatus::Failed && !t.error.empty(),
           "truncated file fails with a reason: " + t.error);
    const fs::path garbage = dir / "garbage.djvu";
    std::ofstream(garbage) << "this is not a djvu file";
    auto g2 = thumtoo::djvu_render_tile_cell(garbage, 1, 0, 0, 0);
    expect(g2.status == TileStatus::Failed && !g2.error.empty(),
           "garbage file fails with a reason: " + g2.error);
  }

  // 9. The document cache keeps 4 files: a fifth evicts the oldest, which
  //    then reopens (counted in opens).
  thumtoo::djvu_release_document_cache();
  thumtoo::djvu_reset_render_stats();
  {
    std::vector<fs::path> copies;
    for (int i = 0; i < 5; ++i) {
      copies.push_back(dir / ("evict" + std::to_string(i) + ".djvu"));
      fs::copy_file(f, copies.back(), fs::copy_options::overwrite_existing);
      (void)thumtoo::djvu_page_count(copies.back());
    }
    (void)thumtoo::djvu_page_count(copies[0]);
    (void)thumtoo::djvu_page_count(copies[4]);
    auto s0 = thumtoo::djvu_document_render_stats(copies[0]);
    auto s4 = thumtoo::djvu_document_render_stats(copies[4]);
    expect(s0 && s0->opens == 2, "evicted document reopened");
    expect(s4 && s4->opens == 1, "recent document stayed open");
  }

  thumtoo::djvu_release_document_cache();
  std::error_code ec;
  fs::remove_all(dir, ec);
  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "djvu_tiles OK\n";
  return 0;
}
