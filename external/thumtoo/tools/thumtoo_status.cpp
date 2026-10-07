// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// Inspect a thumtoo redesign Store cache.

#include "thumtoo/constants.hpp"
#include "thumtoo/layout.hpp"
#include "thumtoo/lqip.hpp"
#include "thumtoo/store.hpp"
#include "thumtoo/uri.hpp"
#include "thumtoo/version.hpp"

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>

namespace fs = std::filesystem;

namespace {

fs::path default_cache_root() {
  const char* xdg = std::getenv("XDG_CACHE_HOME");
  if (xdg && *xdg) {
    return fs::path(xdg) / "thumtoo";
  }
  const char* home = std::getenv("HOME");
  if (home && *home) {
    return fs::path(home) / ".cache" / "thumtoo";
  }
  return fs::path(".cache") / "thumtoo";
}

void usage(const char* argv0) {
  std::cerr
      << "thumtoo " << thumtoo::version_string() << "\n"
      << "Usage:\n"
      << "  " << argv0 << " [--cache DIR] [summary|store]\n"
      << "  " << argv0 << " [--cache DIR] path PATH|URI\n"
      << "\n"
      << "  Inspect a thumtoo Store cache (schema ≥ 100).\n"
      << "  path: locator, size, LQIP, tile scales with have/expected/missing.\n"
      << "  Default DIR: $XDG_CACHE_HOME/thumtoo or ~/.cache/thumtoo\n";
}

void print_store_summary(const fs::path& cache) {
  const auto store_root = thumtoo::redesign_store_root(cache);
  const auto legacy_root = thumtoo::legacy_db_root(cache);
  std::error_code ec;
  const bool legacy_left =
      fs::is_regular_file(legacy_root / "index.sqlite", ec) ||
      fs::is_regular_file(legacy_root / "blobs.sqlite", ec);
  std::cout << "cache:         " << cache << "\n"
            << "layout:        store-root\n"
            << "store_root:    " << store_root << "\n"
            << "legacy_root:   " << legacy_root
            << (legacy_left ? " (files present, ignored by Client)\n"
                            : " (absent)\n");

  if (!fs::is_regular_file(store_root / "index.sqlite", ec)) {
    std::cout << "store:         (no index.sqlite)\n";
    return;
  }
  thumtoo::Store::Paths sp;
  sp.cache_root = store_root;
  sp.data_root = thumtoo::default_data_root();
  auto store = thumtoo::Store::open(sp);
  std::cout << "index_schema:  " << store.index_schema_version() << "\n"
            << "blobs:         " << store.count_blobs() << "\n"
            << "locators:      " << store.count_locators() << "\n"
            << "media:         " << store.count_media() << "\n"
            << "regions:       " << store.count_regions() << "\n"
            << "tiles:         " << store.count_tiles() << "\n";
}

int expected_cells_at_scale(int native_w, int native_h, int scale) {
  const int sw = thumtoo::dim_at_tile_scale(native_w, scale);
  const int sh = thumtoo::dim_at_tile_scale(native_h, scale);
  if (sw <= 0 || sh <= 0) return 0;
  const int nx = (sw + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
  const int ny = (sh + thumtoo::kTileSize - 1) / thumtoo::kTileSize;
  return nx * ny;
}

int theoretical_max_scale(int native_w, int native_h) {
  int w = native_w;
  int h = native_h;
  int s = 0;
  while (w > thumtoo::kTileSize || h > thumtoo::kTileSize) {
    w = thumtoo::dim_at_tile_scale(w, 1);
    h = thumtoo::dim_at_tile_scale(h, 1);
    if (w < 1) w = 1;
    if (h < 1) h = 1;
    ++s;
  }
  return s;
}

void print_file_mtime_wall(const fs::path& path) {
  std::error_code ec;
  const auto ft = fs::last_write_time(path, ec);
  if (ec) return;
  try {
    const auto sctp = std::chrono::file_clock::to_sys(ft);
    const std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    std::cout << "  file_mtime:  " << std::ctime(&tt);  // includes newline
  } catch (...) {
  }
}

int cmd_path_store(const fs::path& cache, std::string_view query) {
  thumtoo::Store::Paths sp;
  sp.cache_root = thumtoo::redesign_store_root(cache);
  sp.data_root = thumtoo::default_data_root();
  std::error_code ec;
  if (!fs::is_regular_file(sp.cache_root / "index.sqlite", ec)) {
    std::cout << "result:        no Store index at " << sp.cache_root << "\n";
    return 0;
  }
  auto store = thumtoo::Store::open(sp);
  std::cout << "query:         " << query << "\n"
            << "store_root:    " << sp.cache_root << "\n";

  std::string uri(query);
  if (query.find("://") == std::string_view::npos && !query.empty() &&
      query[0] == '/') {
    uri = thumtoo::file_uri_from_path(fs::path(std::string(query)));
  }

  auto loc = store.find_locator(uri);
  if (!loc) {
    std::cout << "result:        not cached (no locator)\n";
    return 0;
  }
  std::cout << "locator:\n"
            << "  uri:         " << loc->uri << "\n";
  if (loc->blob_id) std::cout << "  blob_id:     " << *loc->blob_id << "\n";
  if (loc->size) std::cout << "  size:        " << *loc->size << "\n";
  if (loc->mtime_ns) {
    // Opaque file_clock tick fingerprint (invalidate key). On libstdc++ the
    // file_clock epoch is not 1970-01-01, so time_since_epoch().count() is often
    // large and *negative* for real files — not Unix nanoseconds.
    std::cout << "  mtime_ticks: " << *loc->mtime_ns
              << "  (opaque file_clock fingerprint; may be negative)\n";
  }
  if (auto path = thumtoo::path_from_file_uri(uri)) {
    print_file_mtime_wall(*path);
  }

  if (!loc->blob_id) {
    std::cout << "result:        locator without blob\n";
    return 0;
  }

  // LQIP
  {
    auto lqip = store.get_blob_lqip(*loc->blob_id, 0);
    auto kind = store.get_blob_lqip_kind(*loc->blob_id, 0);
    std::cout << "lqip:\n";
    if (!lqip || lqip->empty()) {
      std::cout << "  present:     no\n";
    } else {
      std::cout << "  present:     yes\n"
                << "  bytes:       " << lqip->size() << "\n";
      if (kind) {
        const char* name = "unknown";
        if (*kind == thumtoo::kLqipKindThumbHash) name = "ThumbHash";
        else if (*kind == thumtoo::kLqipKindHandsum) name = "Handsum";
        else if (*kind == thumtoo::kLqipKindNone) name = "none";
        std::cout << "  kind:        " << name << " (" << *kind << ")\n";
      }
    }
  }

  auto media =
      store.find_media_for_blob(*loc->blob_id, thumtoo::MediaKind::Image);
  if (!media) {
    std::cout << "media:         (none — size probe may not have run)\n";
    return 0;
  }

  const int native_w = media->width ? *media->width : 0;
  const int native_h = media->height ? *media->height : 0;
  std::cout << "media:\n"
            << "  id:          " << media->id << "\n"
            << "  kind:        image\n";
  if (native_w > 0 && native_h > 0)
    std::cout << "  size:        " << native_w << "x" << native_h << "\n";

  auto full = store.find_full_region(media->id);
  if (!full) {
    std::cout << "tiles:         (no full region)\n";
    return 0;
  }

  auto scales = store.list_tile_scales(media->id, full->id);
  // List all tile rows (cap high enough for multi-scale pyramids).
  auto rows = store.list_tiles_for_region(media->id, full->id, 500000);
  std::map<int, int> have_by_scale;
  for (const auto& r : rows) {
    ++have_by_scale[r.scale];
  }

  const int theo_max =
      (native_w > 0 && native_h > 0) ? theoretical_max_scale(native_w, native_h)
                                    : -1;

  std::cout << "tiles:\n"
            << "  region_id:   " << full->id << "\n"
            << "  total_cells: " << rows.size() << "\n";
  if (!scales.empty()) {
    std::cout << "  min_scale:   " << scales.front() << "\n"
              << "  max_scale:   " << scales.back() << "\n";
  } else {
    std::cout << "  min_scale:   (none)\n"
              << "  max_scale:   (none)\n";
  }
  if (theo_max >= 0) {
    std::cout << "  theo_max:    " << theo_max
              << "  (single-tile coverage depth)\n";
  }

  if (native_w <= 0 || native_h <= 0) {
    std::cout << "  per_scale:   (need media size for expected counts)\n";
    if (!have_by_scale.empty()) {
      for (const auto& [sc, n] : have_by_scale) {
        std::cout << "    scale " << sc << ": have=" << n << "\n";
      }
    }
    return 0;
  }

  // Report every theoretical scale 0..theo_max plus any extra stored scales.
  int missing_total = 0;
  int expected_total = 0;
  std::cout << "  per_scale:\n";
  std::map<int, bool> reported;
  for (int sc = 0; sc <= theo_max; ++sc) {
    const int expect = expected_cells_at_scale(native_w, native_h, sc);
    const int have = have_by_scale.count(sc) ? have_by_scale[sc] : 0;
    const int miss = expect > have ? expect - have : 0;
    expected_total += expect;
    missing_total += miss;
    std::cout << "    scale " << sc << ": have=" << have << " expected=" << expect
              << " missing=" << miss;
    if (expect > 0 && have >= expect) std::cout << "  OK";
    else if (have == 0) std::cout << "  empty";
    else std::cout << "  partial";
    std::cout << "\n";
    reported[sc] = true;
  }
  for (const auto& [sc, n] : have_by_scale) {
    if (reported.count(sc)) continue;
    std::cout << "    scale " << sc << ": have=" << n
              << " expected=? (outside theo range)\n";
  }
  std::cout << "  summary:     have=" << rows.size() << " expected=" << expected_total
            << " missing=" << missing_total;
  if (missing_total == 0 && expected_total > 0)
    std::cout << "  complete pyramid";
  else if (rows.empty())
    std::cout << "  no tiles — run: thumtoo-prepare --tiles <path>";
  else if (missing_total > 0)
    std::cout << "  incomplete";
  std::cout << "\n";

  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  fs::path cache = default_cache_root();
  std::string mode = "summary";
  std::optional<std::string> path_query;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "--version" || a == "-V") {
      thumtoo::print_version(std::cout);
      return 0;
    }
    if (a == "--help" || a == "-h") {
      usage(argv[0]);
      return 0;
    }
    if (a == "--cache" && i + 1 < argc) {
      cache = argv[++i];
      continue;
    }
    if (a == "path" || a == "query" || a == "show") {
      mode = "path";
      if (i + 1 >= argc) {
        std::cerr << "error: path requires PATH or URI\n";
        usage(argv[0]);
        return 2;
      }
      path_query = argv[++i];
      continue;
    }
    if (a == "summary" || a == "store") {
      mode = a;
      continue;
    }
    if (a == "legacy" || a == "locators" || a == "content" || a == "levels" ||
        a == "tiles" || a == "archives") {
      std::cerr << "thumtoo-status: mode '" << a
                << "' removed (schema-4 ladder is gone)\n"
                << "  Use summary or path.\n";
      return 2;
    }
    std::cerr << "Unknown argument: " << a << "\n";
    usage(argv[0]);
    return 2;
  }

  try {
    if (mode == "path") {
      return cmd_path_store(cache, *path_query);
    }
    print_store_summary(cache);
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
