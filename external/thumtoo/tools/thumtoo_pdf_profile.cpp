// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

// thumtoo-pdf-profile — how thumtoo classifies and renders PDF pages.
//
//   thumtoo-pdf-profile [--json] [--render SCALE|cap] [--threads N]
//                       FILE.pdf [PAGE...]
//
// Prints each page's PdfPageProfile (kind, images and their dpi, vector and
// glyph counts, resolution cap, summary). --render renders every cell of the
// page at SCALE (or at the page's own cap / layout scale with "cap") and
// reports cells, time, display-list builds and exact image decode counts —
// the decode-once benchmark.

#include "thumtoo/constants.hpp"
#include "thumtoo/pdf.hpp"
#include "thumtoo/types.hpp"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

std::string json_escape(const std::string& s) {
  std::string out;
  for (char c : s) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      default:
        if (static_cast<unsigned char>(c) < 0x20) {
          char buf[8];
          std::snprintf(buf, sizeof buf, "\\u%04x", c);
          out += buf;
        } else {
          out += c;
        }
    }
  }
  return out;
}

struct RenderResult {
  int scale = 0;
  int cells = 0;
  int ok = 0;
  int unavailable = 0;
  int failed = 0;
  double wall_ms = 0.0;
  std::string first_error;
  std::optional<thumtoo::PdfPageRenderStats> stats;
};

RenderResult render_page(const std::filesystem::path& pdf, int page, int scale,
                         int threads) {
  RenderResult r;
  r.scale = scale;
  auto layout = thumtoo::pdf_page_layout_size(pdf, page);
  if (!layout) {
    r.first_error = "no layout size";
    return r;
  }
  const auto full = thumtoo::pdf_page_size_at_scale(*layout, scale);
  const int nx = (full.width + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
  const int ny = (full.height + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
  r.cells = nx * ny;
  std::atomic<int> next{0}, ok{0}, unavailable{0}, failed{0};
  std::mutex mu;
  const auto t0 = std::chrono::steady_clock::now();
  std::vector<std::thread> pool;
  for (int t = 0; t < std::max(1, threads); ++t) {
    pool.emplace_back([&] {
      for (int i = next++; i < r.cells; i = next++) {
        auto cell = thumtoo::pdf_render_tile_cell(pdf, page, scale, i % nx, i / nx);
        switch (cell.status) {
          case thumtoo::TileStatus::Ok: ++ok; break;
          case thumtoo::TileStatus::Unavailable: ++unavailable; break;
          default: ++failed; break;
        }
        if (cell.status != thumtoo::TileStatus::Ok) {
          std::lock_guard lock(mu);
          if (r.first_error.empty()) r.first_error = cell.error;
        }
      }
    });
  }
  for (auto& t : pool) t.join();
  r.wall_ms = std::chrono::duration<double, std::milli>(
                  std::chrono::steady_clock::now() - t0)
                  .count();
  r.ok = ok;
  r.unavailable = unavailable;
  r.failed = failed;
  if (auto st = thumtoo::pdf_document_render_stats(pdf)) {
    for (const auto& p : st->pages) {
      if (p.page == page) r.stats = p;
    }
  }
  return r;
}

void usage() {
  std::cerr << "usage: thumtoo-pdf-profile [--json] [--render SCALE|cap] "
               "[--threads N] FILE.pdf [PAGE...]\n";
}

}  // namespace

int main(int argc, char** argv) {
  bool json = false;
  std::optional<std::string> render;
  int threads = 1;
  std::filesystem::path pdf;
  std::vector<int> pages;
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--json") {
      json = true;
    } else if (a == "--render" && i + 1 < argc) {
      render = argv[++i];
    } else if (a == "--threads" && i + 1 < argc) {
      threads = std::max(1, std::atoi(argv[++i]));
    } else if (a == "-h" || a == "--help") {
      usage();
      return 0;
    } else if (pdf.empty()) {
      pdf = a;
    } else {
      pages.push_back(std::atoi(a.c_str()));
    }
  }
  if (pdf.empty()) {
    usage();
    return 2;
  }
  const auto count = thumtoo::pdf_page_count(pdf);
  if (!count) {
    std::cerr << pdf << ": cannot open as PDF\n";
    return 1;
  }
  if (pages.empty()) {
    for (int p = 1; p <= *count; ++p) pages.push_back(p);
  }

  int failures = 0;
  if (json) std::cout << "{\"file\":\"" << json_escape(pdf.string()) << "\",\"pages\":[";
  bool first = true;
  for (int page : pages) {
    std::string error;
    thumtoo::pdf_reset_render_stats();
    auto prof = thumtoo::pdf_page_profile(pdf, page, &error);
    std::optional<RenderResult> rr;
    if (prof && render) {
      const int scale = *render == "cap" ? prof->finest_useful_scale.value_or(0)
                                         : std::atoi(render->c_str());
      rr = render_page(pdf, page, scale, threads);
      failures += rr->failed;
    }
    if (!prof) ++failures;
    if (json) {
      std::ostringstream o;
      o << (first ? "" : ",") << "{\"page\":" << page;
      if (!prof) {
        o << ",\"error\":\"" << json_escape(error) << "\"}";
      } else {
        o << ",\"kind\":\"" << thumtoo::page_content_kind_name(prof->kind) << "\""
          << ",\"width_pt\":" << prof->width_pt << ",\"height_pt\":" << prof->height_pt
          << ",\"native_dpi\":" << prof->native_dpi << ",\"finest_useful_scale\":"
          << (prof->finest_useful_scale ? std::to_string(*prof->finest_useful_scale)
                                        : std::string("null"))
          << ",\"image_draws\":" << prof->image_draws
          << ",\"image_coverage\":" << prof->image_coverage
          << ",\"vector_paths\":" << prof->vector_paths << ",\"shadings\":" << prof->shadings
          << ",\"visible_glyphs\":" << prof->visible_glyphs
          << ",\"clip_glyphs\":" << prof->clip_glyphs
          << ",\"invisible_glyphs\":" << prof->invisible_glyphs
          << ",\"background_fill_ignored\":"
          << (prof->background_fill_ignored ? "true" : "false") << ",\"images\":[";
        for (std::size_t i = 0; i < prof->images.size(); ++i) {
          const auto& im = prof->images[i];
          o << (i ? "," : "") << "{\"width\":" << im.width << ",\"height\":" << im.height
            << ",\"components\":" << im.components << ",\"bpc\":" << im.bpc
            << ",\"stencil\":" << (im.stencil ? "true" : "false") << ",\"dpi\":" << im.dpi
            << ",\"page_fraction\":" << im.page_fraction << "}";
        }
        o << "],\"summary\":\"" << json_escape(prof->summary) << "\"";
        if (rr) {
          o << ",\"render\":{\"scale\":" << rr->scale << ",\"threads\":" << threads
            << ",\"cells\":" << rr->cells << ",\"ok\":" << rr->ok
            << ",\"unavailable\":" << rr->unavailable << ",\"failed\":" << rr->failed
            << ",\"wall_ms\":" << rr->wall_ms;
          if (rr->stats) {
            const auto& s = *rr->stats;
            o << ",\"render_ms\":" << s.render_ms
              << ",\"display_list_builds\":" << s.display_list_builds
              << ",\"decodes\":" << s.decode.decodes
              << ",\"full_decodes\":" << s.decode.full_decodes
              << ",\"subarea_decodes\":" << s.decode.subarea_decodes
              << ",\"decoded_pixels\":" << s.decode.decoded_pixels
              << ",\"decode_ms\":" << s.decode.decode_ms
              << ",\"shared_waits\":" << s.decode.shared_waits;
          }
          o << ",\"first_error\":\"" << json_escape(rr->first_error) << "\"}";
        }
        o << "}";
      }
      std::cout << o.str();
      first = false;
      continue;
    }
    std::cout << "page " << page << ": ";
    if (!prof) {
      std::cout << "ERROR " << error << "\n";
      continue;
    }
    std::cout << prof->summary << "\n";
    for (const auto& im : prof->images) {
      std::printf("    image %dx%d n=%d bpc=%d%s  %.0f dpi  %.0f%% of page\n", im.width,
                  im.height, im.components, im.bpc, im.stencil ? " stencil" : "", im.dpi,
                  im.page_fraction * 100.0);
    }
    if (rr) {
      std::printf("    render s=%d: %d cells (%d ok, %d unavailable, %d failed) in %.1f ms "
                  "with %d thread%s\n",
                  rr->scale, rr->cells, rr->ok, rr->unavailable, rr->failed, rr->wall_ms,
                  threads, threads == 1 ? "" : "s");
      if (rr->stats) {
        const auto& s = *rr->stats;
        std::printf("    display lists %lld (%.1f ms)  decodes %lld (full %lld, subarea "
                    "%lld, %.1f MP, %.1f ms), %lld waited for a shared decode\n",
                    static_cast<long long>(s.display_list_builds), s.display_list_ms,
                    static_cast<long long>(s.decode.decodes),
                    static_cast<long long>(s.decode.full_decodes),
                    static_cast<long long>(s.decode.subarea_decodes),
                    s.decode.decoded_pixels / 1e6, s.decode.decode_ms,
                    static_cast<long long>(s.decode.shared_waits));
        if (!s.decode.last_subarea_reason.empty()) {
          std::cout << "    subarea: " << s.decode.last_subarea_reason << "\n";
        }
      }
      if (!rr->first_error.empty()) std::cout << "    first error: " << rr->first_error << "\n";
    }
  }
  if (json) std::cout << "]}\n";
  return failures ? 1 : 0;
}
