// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// EPUB on the shared MuPDF runtime, with tests/fixtures/book.epub
/// (make_epub_fixture.py: cover image page, long text chapter, figure page).
///
/// * One layout per (file, layout) for the whole process — not per worker
///   thread (`opens` stays 1 across threads); a different layout is a
///   different document with a different page count.
/// * Page profiles: cover Raster (background fill ignored), text Vector,
///   figure page Mixed.
/// * All cells of the cover decode its image once; text cells stitch
///   byte-exactly; deep zoom on text renders.
/// * Errors carry reasons; text layer and outline still work.

#include "thumtoo/epub.hpp"
#include "thumtoo/constants.hpp"

#include <algorithm>
#include <atomic>
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

}  // namespace

int main() {
  const fs::path book = fs::path(THUMTOO_FIXTURES_DIR) / "book.epub";
  const auto L = thumtoo::default_epub_layout();
  thumtoo::pdf_reset_render_stats();

  // 1. Layout and profiles.
  const int pages = thumtoo::epub_page_count(book, L).value_or(0);
  expect(pages >= 4, "page count " + std::to_string(pages));
  {
    std::string e;
    auto cover = thumtoo::epub_page_profile(book, 1, L, &e);
    expect(cover && cover->kind == PageContentKind::Raster,
           "cover is raster: " + (cover ? cover->summary : e));
    expect(cover && cover->background_fill_ignored, "cover: CSS page background ignored");
    expect(cover && cover->finest_useful_scale.has_value(), "cover is capped");
    auto text = thumtoo::epub_page_profile(book, 2, L, &e);
    expect(text && text->kind == PageContentKind::Vector && text->visible_glyphs > 100,
           "chapter page is vector text: " + (text ? text->summary : e));
    auto last = thumtoo::epub_page_profile(book, pages, L, &e);
    expect(last && last->kind == PageContentKind::Mixed,
           "figure page is mixed: " + (last ? last->summary : e));
  }

  // 2. Threads share one laid-out document.
  {
    std::atomic<int> bad{0};
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
      threads.emplace_back([&, t] {
        for (int p = 1 + t; p <= pages; p += 4) {
          auto size = thumtoo::epub_page_layout_size(book, p, L);
          if (!size) {
            ++bad;
            continue;
          }
          if (thumtoo::epub_render_tile_cell(book, p, L, 0, 0, 0).status != TileStatus::Ok) {
            ++bad;
          }
        }
      });
    }
    for (auto& th : threads) th.join();
    expect(bad == 0, "threaded cells render");
    auto st = thumtoo::epub_document_render_stats(book);
    expect(st && st->opens == 1,
           "one layout for all threads, opens=" + std::to_string(st ? st->opens : -1));
  }

  // 3. Another layout is another document (bigger font → more pages).
  {
    auto big = L;
    big.fs_pt = L.fs_pt * 2;
    const int big_pages = thumtoo::epub_page_count(book, big).value_or(0);
    expect(big_pages > pages, "larger font, more pages: " + std::to_string(big_pages));
    auto st = thumtoo::epub_document_render_stats(book);
    expect(st && st->opens == 2, "second layout opened once");
  }

  // 4. Decode once: all cover cells at scale 0 share one image decode. A
  //    layout no cell was rendered with yet, so nothing is in the store.
  thumtoo::pdf_reset_render_stats();
  {
    auto fresh = L;
    fresh.width_px = L.width_px + 100;
    const auto size = *thumtoo::epub_page_layout_size(book, 1, fresh);
    const int nx = (size.width + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
    const int ny = (size.height + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
    int ok = 0;
    for (int y = 0; y < ny; ++y) {
      for (int x = 0; x < nx; ++x) {
        ok += thumtoo::epub_render_tile_cell(book, 1, fresh, 0, x, y).status == TileStatus::Ok;
      }
    }
    expect(ok == nx * ny, "all cover cells");
    auto st = thumtoo::epub_document_render_stats(book);
    const auto* p = st ? page_stats(*st, 1) : nullptr;
    expect(p && p->decode.decodes == 1,
           "one cover decode for " + std::to_string(nx * ny) + " cells, got " +
               std::to_string(p ? p->decode.decodes : -1));
  }

  // 5. Text page: stitched cells == one region render; deep zoom renders.
  {
    const auto size = *thumtoo::epub_page_layout_size(book, 2, L);
    std::vector<std::uint8_t> stitched(static_cast<std::size_t>(size.width) * size.height * 3);
    for (int ty = 0; ty * thumtoo::kTileSize < size.height; ++ty) {
      for (int tx = 0; tx * thumtoo::kTileSize < size.width; ++tx) {
        auto c = thumtoo::epub_render_tile_cell(book, 2, L, 0, tx, ty);
        if (!c.raster) continue;
        for (int y = 0; y < c.raster->height; ++y) {
          std::copy_n(&c.raster->rgb[static_cast<std::size_t>(y) * c.raster->width * 3],
                      c.raster->width * 3,
                      &stitched[(static_cast<std::size_t>(ty * thumtoo::kTileSize + y) *
                                     size.width +
                                 tx * thumtoo::kTileSize) * 3]);
        }
      }
    }
    auto whole = thumtoo::epub_rasterize_page_region(book, 2, L, thumtoo::kEpubLayoutDpi, 0, 0,
                                                     size.width, size.height);
    expect(whole && whole->rgb == stitched, "text cells stitch exactly");
    auto deep = thumtoo::epub_render_tile_cell(book, 2, L, -4, 20, 30);
    expect(deep.status == TileStatus::Ok, "text page at -4: " + deep.error);
    auto capped = thumtoo::epub_render_tile_cell(book, 1, L, -3, 0, 0);
    expect(capped.status == TileStatus::Unavailable && !capped.error.empty(),
           "cover below its cap: " + capped.error);
  }

  // 6. Reasons, text layer, outline.
  {
    auto missing = thumtoo::epub_render_tile_cell("/nonexistent/x.epub", 1, L, 0, 0, 0);
    expect(missing.status == TileStatus::Failed && !missing.error.empty(),
           "missing file: " + missing.error);
    auto past = thumtoo::epub_render_tile_cell(book, pages + 5, L, 0, 0, 0);
    expect(past.status == TileStatus::Failed &&
               past.error.find("out of range") != std::string::npos,
           "page out of range: " + past.error);
    auto layer = thumtoo::epub_page_text_layer(book, 2, L);
    expect(layer && !layer->regions.empty(), "text layer has lines");
    auto outline = thumtoo::epub_document_outline(book, L);
    expect(outline && outline->items.size() >= 2, "outline has the two chapters");
    if (outline && outline->items.size() >= 2) {
      // Locations are (chapter, page-in-chapter); pages must be absolute.
      expect(outline->items[0].page_1based == 2,
             "chapter one starts on page 2, got " +
                 std::to_string(outline->items[0].page_1based));
      expect(outline->items[1].page_1based == pages,
             "chapter two starts on the last page, got " +
                 std::to_string(outline->items[1].page_1based));
    }
  }

  thumtoo::pdf_release_document_cache();
  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "epub_tiles OK\n";
  return 0;
}
