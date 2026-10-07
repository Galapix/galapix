// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// Cache maintenance for redesign Store (schema >= 100). Schema-4 ladder GC
/// was removed with Database/BlobStore.

#include "thumtoo/layout.hpp"
#include "thumtoo/version.hpp"
#include "thumtoo/store.hpp"
#include "thumtoo/uri.hpp"

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

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
      << "Usage: " << argv0
      << " [--cache DIR] [--dry-run]\n"
      << "       [--uri URI]... [--path PATH]...\n"
      << "       [--store-summary] [--orphans] [--uri-prefix PREFIX]\n"
      << "\n"
      << "Manual Store maintenance (no automatic eviction).\n"
      << "\n"
      << "  --cache DIR       Cache root (default: $XDG_CACHE_HOME/thumtoo)\n"
      << "  --dry-run         Report only; do not delete\n"
      << "  --uri URI         Forget this location URI (locator + orphan blob/tiles)\n"
      << "  --path PATH       Forget file:// locator for this filesystem path\n"
      << "  --uri-prefix P    Forget all locators whose uri starts with P\n"
      << "  --orphans         Purge Store blobs with no locator references\n"
      << "  --store-summary   Print Store blob/locator/tile counts\n"
      << "\n"
      << "Schema-4 ladder flags (--soft-levels / --dead-paths / --min-scale) were\n"
      << "removed with the legacy Database. On-disk legacy/ trees are left untouched.\n";
}

void print_store_summary(const fs::path& cache) {
  thumtoo::Store::Paths sp;
  sp.cache_root = thumtoo::redesign_store_root(cache);
  sp.data_root = thumtoo::default_data_root();
  std::error_code ec;
  if (!fs::is_regular_file(sp.cache_root / "index.sqlite", ec)) {
    std::cout << "store:         (no index.sqlite at " << sp.cache_root << ")\n";
    return;
  }
  auto store = thumtoo::Store::open(sp);
  std::cout << "store_root:    " << sp.cache_root << "\n"
            << "index_schema:  " << store.index_schema_version() << "\n"
            << "blobs:         " << store.count_blobs() << "\n"
            << "locators:      " << store.count_locators() << "\n"
            << "media:         " << store.count_media() << "\n"
            << "regions:       " << store.count_regions() << "\n"
            << "tiles:         " << store.count_tiles() << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  fs::path cache = default_cache_root();
  bool dry_run = false;
  bool do_store_summary = false;
  bool do_orphans = false;
  std::vector<std::string> uris;
  std::vector<std::string> uri_prefixes;
  std::vector<fs::path> paths;

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
    if (a == "--dry-run") {
      dry_run = true;
      continue;
    }
    if (a == "--uri" && i + 1 < argc) {
      uris.emplace_back(argv[++i]);
      continue;
    }
    if (a == "--path" && i + 1 < argc) {
      paths.emplace_back(argv[++i]);
      continue;
    }
    if (a == "--uri-prefix" && i + 1 < argc) {
      uri_prefixes.emplace_back(argv[++i]);
      continue;
    }
    if (a == "--store-summary") {
      do_store_summary = true;
      continue;
    }
    if (a == "--orphans") {
      do_orphans = true;
      continue;
    }
    if (a == "--dead-paths" || a == "--soft-levels" || a == "--min-scale") {
      std::cerr << "thumtoo-gc: " << a
                << " removed (schema-4 ladder GC is gone)\n"
                << "  Use --uri / --path / --uri-prefix / --orphans / --store-summary.\n";
      return 2;
    }
    std::cerr << "Unknown argument: " << a << "\n";
    usage(argv[0]);
    return 2;
  }

  if (!do_store_summary && !do_orphans && uris.empty() && paths.empty()
      && uri_prefixes.empty()) {
    usage(argv[0]);
    return 2;
  }

  try {
    std::cout << "cache: " << cache << (dry_run ? " (dry-run)\n" : "\n");
    if (do_store_summary) {
      print_store_summary(cache);
    }

    if (uris.empty() && paths.empty() && uri_prefixes.empty() && !do_orphans) {
      return 0;
    }

    thumtoo::Store::Paths sp;
    sp.cache_root = thumtoo::redesign_store_root(cache);
    sp.data_root = thumtoo::default_data_root();
    std::error_code ec;
    if (!fs::is_regular_file(sp.cache_root / "index.sqlite", ec)) {
      std::cerr << "thumtoo-gc: no Store index at " << sp.cache_root << "\n";
      return 2;
    }
    auto store = thumtoo::Store::open(sp);
    if (!uris.empty() || !paths.empty()) {
      std::cout << "store forget:\n";
      for (const auto& u : uris) {
        auto st = store.forget_uri(u, dry_run);
        std::cout << "  uri " << u
                  << (st.locator_removed ? " removed" : " miss")
                  << (st.blob_purged ? " (blob purged)" : "")
                  << " tiles=" << st.tiles_deleted
                  << (dry_run ? " (dry-run)\n" : "\n");
      }
      for (const auto& p : paths) {
        std::error_code e2;
        auto abs = fs::absolute(p, e2);
        if (e2) abs = p;
        const auto file_uri =
            thumtoo::file_uri_from_path(abs.lexically_normal());
        auto st = store.forget_uri(file_uri, dry_run);
        std::cout << "  path " << p << " -> " << file_uri
                  << (st.locator_removed ? " removed" : " miss")
                  << (st.blob_purged ? " (blob purged)" : "")
                  << " tiles=" << st.tiles_deleted
                  << (dry_run ? " (dry-run)\n" : "\n");
      }
    }
    for (const auto& pref : uri_prefixes) {
      auto pst = store.forget_uri_prefix(pref, dry_run);
      std::cout << "uri-prefix " << pref
                << " locators=" << pst.locators_removed
                << " blobs=" << pst.blobs_purged
                << " tiles=" << pst.tiles_deleted
                << (dry_run ? " (dry-run)\n" : "\n");
    }
    if (do_orphans) {
      auto ost = store.purge_orphan_blobs(dry_run);
      std::cout << "orphan blobs: " << ost.blobs_purged
                << " tiles=" << ost.tiles_deleted
                << (dry_run ? " (dry-run)\n" : "\n");
    }
  } catch (const std::exception& e) {
    std::cerr << "error: " << e.what() << "\n";
    return 1;
  }
  return 0;
}
