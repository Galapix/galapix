// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// Headless open-path timing (no SDL/viewer). Matches ViewerCommand phases so
// we measure where seconds go instead of guessing about throttles.
//
// Usage:
//   open_phase_bench [--cache DIR] [--no-tiles] archive_or_image...
//
// Requires HAVE_THUMTOO. Warm cache: run thumtoo-prepare / thumtoo-bench first.

#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#ifdef HAVE_THUMTOO
#  include <thumtoo/archive.hpp>
#  include <thumtoo/client.hpp>
#  include <thumtoo/image.hpp>
#  include <thumtoo/pdf.hpp>

#  include "thumtoo/thumtoo_callback_queue.hpp"
#  include "thumtoo/thumtoo_tile_provider.hpp"
#  include "thumtoo/thumtoo_uri.hpp"
#  include "util/url.hpp"
#  include "math/vector2i.hpp"
#endif

namespace {

using clock = std::chrono::steady_clock;

struct Phase {
  char const* name;
  double seconds = 0;
  std::string note;
};

double sec_since(clock::time_point t0)
{
  return std::chrono::duration<double>(clock::now() - t0).count();
}

void print_table(std::vector<Phase> const& phases)
{
  double total = 0;
  for (auto const& p : phases) {
    total += p.seconds;
  }
  std::cout << "\n=== open_phase_bench ===\n";
  for (auto const& p : phases) {
    double pct = total > 0 ? 100.0 * p.seconds / total : 0;
    std::cout << "  " << p.name << ": " << p.seconds << " s"
              << " (" << static_cast<int>(std::lround(pct)) << "%)";
    if (!p.note.empty()) {
      std::cout << "  [" << p.note << "]";
    }
    std::cout << "\n";
  }
  std::cout << "  TOTAL: " << total << " s\n";
}

} // namespace

int main(int argc, char** argv)
{
#ifndef HAVE_THUMTOO
  std::cerr << "open_phase_bench requires HAVE_THUMTOO\n";
  return 1;
#else
  namespace fs = std::filesystem;
  fs::path cache;
  bool do_tiles = true;
  std::vector<fs::path> paths;

  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a == "-h" || a == "--help") {
      std::cout
        << "Usage: open_phase_bench [--cache DIR] [--no-tiles] PATH...\n"
        << "  Times expand / size-probe / provider create / overview tiles\n"
        << "  (headless; no GL). Compare warm vs cold thumtoo cache.\n";
      return 0;
    }
    if (a == "--cache" && i + 1 < argc) {
      cache = argv[++i];
      continue;
    }
    if (a == "--no-tiles") {
      do_tiles = false;
      continue;
    }
    paths.emplace_back(a);
  }

  if (paths.empty()) {
    std::cerr << "open_phase_bench: need at least one path\n";
    return 1;
  }

  if (cache.empty()) {
    if (char const* home = std::getenv("HOME")) {
      cache = fs::path(home) / ".cache" / "thumtoo";
    } else {
      cache = fs::temp_directory_path() / "thumtoo-open-bench";
    }
  }

  thumtoo::image_library_init();
  auto client = thumtoo::Client::open(
    cache, galapix::ThumtooCallbackQueue::instance().make_executor());
  std::cout << "cache: " << cache << "\n";

  std::vector<Phase> phases;
  std::vector<std::string> uris;

  // --- expand archives / PDF ---
  {
    auto t0 = clock::now();
    for (auto const& path : paths) {
      galapix::URL url = galapix::URL::from_filename(path.string());
      // Mirror expand_thumtoo_urls without pulling private ViewerCommand.
      if (thumtoo::is_likely_pdf_path(path)) {
        auto pages = thumtoo::pdf_page_count(path);
        if (pages && *pages > 0) {
          for (int p = 0; p < *pages; ++p) {
            uris.push_back("file://" + path.string() + "//pdf:" +
                           std::to_string(p));
          }
          continue;
        }
      }
      if (thumtoo::is_likely_archive_path(path)) {
        auto toc = thumtoo::read_archive_toc(path);
        if (toc) {
          for (auto const& mem : *toc) {
            if (mem.member_path.empty()) {
              continue;
            }
            uris.push_back("file://" + path.string() +
                           "//archive:" + mem.member_path);
          }
          continue;
        }
      }
      uris.push_back(galapix::thumtoo_uri_from_url(url));
    }
    phases.push_back(
      {"expand", sec_since(t0),
       std::to_string(paths.size()) + " path(s) -> " +
         std::to_string(uris.size()) + " uri(s)"});
  }

  // --- batch size probe ---
  {
    auto t0 = clock::now();
    int pending = 0;
    int already = 0;
    for (auto const& uri : uris) {
      if (uri.empty()) {
        continue;
      }
      if (client->get_size(uri)) {
        ++already;
        continue;
      }
      ++pending;
      client->request_size(uri, [](std::string, std::optional<thumtoo::Size>) {});
    }
    if (pending > 0) {
      client->drain();
      galapix::ThumtooCallbackQueue::instance().pump();
    }
    phases.push_back(
      {"size_probe", sec_since(t0),
       std::to_string(already) + " cached, " + std::to_string(pending) +
         " probed"});
  }

  // --- create providers (like make_file_tile_provider + Image ctor path) ---
  std::vector<galapix::TileProviderPtr> providers;
  {
    auto t0 = clock::now();
    int ok = 0;
    for (auto const& uri : uris) {
      if (uri.empty()) {
        continue;
      }
      galapix::TileProviderPtr p;
      if (auto sz = client->get_size(uri)) {
        p = galapix::ThumtooTileProvider::create_from_size(
          client, uri, sz->width, sz->height);
      }
      if (p) {
        providers.push_back(std::move(p));
        ++ok;
      }
    }
    phases.push_back(
      {"providers", sec_since(t0), std::to_string(ok) + " providers"});
  }

  // --- request overview tile (max_scale, 0,0) for each provider ---
  if (do_tiles && !providers.empty()) {
    auto t0 = clock::now();
    std::atomic<int> remaining{0};
    int issued = 0;
    for (auto const& p : providers) {
      int const scale = p->get_max_scale();
      ++issued;
      remaining.fetch_add(1);
      p->request_tile(
        scale, galapix::Vector2i(0, 0),
        [&remaining](galapix::Tile const&) {
          remaining.fetch_sub(1);
        });
    }
    client->drain();
    galapix::ThumtooCallbackQueue::instance().pump();
    // Drain may return before all executor callbacks if any left; spin briefly.
    auto deadline = clock::now() + std::chrono::seconds(120);
    while (remaining.load() > 0 && clock::now() < deadline) {
      client->drain();
      galapix::ThumtooCallbackQueue::instance().pump();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    int left = remaining.load();
    phases.push_back(
      {"overview_tiles", sec_since(t0),
       std::to_string(issued) + " requested, " + std::to_string(left) +
         " unfinished"});
  }

  print_table(phases);
  return 0;
#endif
}
