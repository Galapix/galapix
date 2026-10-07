// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

/// Top-level Store layout + dual-path migrate (nested store/ opt-out removed).

#include "thumtoo/client.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/store.hpp"

#include "sqlite3.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include <unistd.h>

namespace fs = std::filesystem;

namespace {

int g_failures = 0;

void expect(bool cond, const char* msg) {
  if (!cond) {
    std::cerr << "FAIL: " << msg << "\n";
    ++g_failures;
  }
}

fs::path make_tmpdir(const char* suffix) {
  const fs::path base =
      fs::temp_directory_path() / (std::string("thumtoo-") + suffix + "-XXXXXX");
  std::string tmpl = base.string();
  std::vector<char> buf(tmpl.begin(), tmpl.end());
  buf.push_back('\0');
  if (!mkdtemp(buf.data())) {
    std::perror("mkdtemp");
    std::exit(1);
  }
  return fs::path(buf.data());
}

bool file_nonempty(const fs::path& p) {
  std::error_code ec;
  return fs::is_regular_file(p, ec) && fs::file_size(p, ec) > 0;
}

/// Minimal schema_meta seed for classic dual-path files (no Database/BlobStore).
void seed_schema_meta(const fs::path& db_path, int version) {
  std::error_code ec;
  fs::create_directories(db_path.parent_path(), ec);
  sqlite3* db = nullptr;
  if (sqlite3_open(db_path.string().c_str(), &db) != SQLITE_OK) {
    if (db) sqlite3_close(db);
    expect(false, "seed open sqlite");
    return;
  }
  char* err = nullptr;
  // DELETE journal avoids leftover -wal that could confuse exists checks.
  sqlite3_exec(db, "PRAGMA journal_mode=DELETE;", nullptr, nullptr, &err);
  if (err) {
    sqlite3_free(err);
    err = nullptr;
  }
  sqlite3_exec(db,
               "CREATE TABLE IF NOT EXISTS schema_meta("
               "key TEXT PRIMARY KEY, value TEXT NOT NULL);",
               nullptr, nullptr, &err);
  if (err) {
    sqlite3_free(err);
    err = nullptr;
  }
  const std::string sql =
      "INSERT OR REPLACE INTO schema_meta(key, value) VALUES('schema_version', '" +
      std::to_string(version) + "');";
  sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
  if (err) sqlite3_free(err);
  sqlite3_exec(db, "PRAGMA wal_checkpoint(TRUNCATE);", nullptr, nullptr, nullptr);
  sqlite3_close(db);
}

void dump_tree(const fs::path& root, const char* label) {
  std::error_code ec;
  std::cerr << "  [" << label << "] " << root << "\n";
  if (!fs::exists(root, ec)) {
    std::cerr << "    (missing)\n";
    return;
  }
  for (const auto& e : fs::recursive_directory_iterator(root, ec)) {
    std::cerr << "    " << e.path().lexically_relative(root).string();
    if (e.is_regular_file(ec)) {
      std::cerr << " (" << fs::file_size(e.path(), ec) << " B)";
    } else if (e.is_directory(ec)) {
      std::cerr << "/";
    }
    std::cerr << "\n";
  }
}

}  // namespace

int main() {
  unsetenv("THUMTOO_STORE_ROOT");  // ignored; top-level Store always
  unsetenv("THUMTOO_STORE_ONLY");

  // --- A: fresh open places Store at cache root; no legacy Client open ---
  {
    const fs::path cache = make_tmpdir("store-root-fresh");
    auto client = thumtoo::Client::open(cache, {}, 0, cache);
    expect(client != nullptr, "Client::open top-level Store");
    expect(file_nonempty(cache / "index.sqlite"),
           "fresh: Store index at cache root");
    expect(file_nonempty(cache / "bulk.sqlite"),
           "fresh: Store bulk at cache root");
    expect(!fs::exists(cache / "legacy"), "fresh: no legacy/ from Client");
    expect(!fs::exists(cache / "store" / "index.sqlite"),
           "fresh: no store/ subdirectory index");
  }

  // --- B: classic dual-path cache migrates on open ---
  {
    const fs::path cache = make_tmpdir("store-root-migrate");
    // Seed legacy at top (schema < 100) without Database/BlobStore classes.
    {
      seed_schema_meta(cache / "index.sqlite", thumtoo::kSchemaVersion);
      seed_schema_meta(cache / "blobs.sqlite", thumtoo::kSchemaVersion);
      expect(file_nonempty(cache / "index.sqlite"), "seed legacy index.sqlite");
      expect(file_nonempty(cache / "blobs.sqlite"), "seed legacy blobs.sqlite");
    }
    // Seed redesign under store/ (schema ≥ 100).
    {
      thumtoo::Store::Paths sp;
      sp.cache_root = cache / "store";
      sp.data_root = cache / "store";
      auto store = thumtoo::Store::open(sp);
      expect(store.index_schema_version() == thumtoo::kStoreIndexSchemaVersion,
             "seed store schema");
      const auto blob_id = store.insert_blob(std::optional<std::int64_t>{42},
                                         thumtoo::BlobStatus::Ok);
      expect(blob_id >= 1, "seed store blob");
    }
    expect(file_nonempty(cache / "index.sqlite"), "pre: top legacy index");
    expect(file_nonempty(cache / "blobs.sqlite"), "pre: top legacy blobs");
    expect(file_nonempty(cache / "store" / "index.sqlite"), "pre: store/ index");

    auto client = thumtoo::Client::open(cache, {}, 0, cache);
    expect(client != nullptr, "Client::open migrates dual-path");

    const bool legacy_index_ok =
        file_nonempty(cache / "legacy" / "index.sqlite");
    const bool legacy_blobs_ok =
        file_nonempty(cache / "legacy" / "blobs.sqlite");
    expect(legacy_index_ok, "migrate: legacy index under legacy/ (on disk)");
    expect(legacy_blobs_ok, "migrate: legacy blobs under legacy/ (on disk)");
    if (!legacy_index_ok || !legacy_blobs_ok) {
      dump_tree(cache, "post-migrate cache tree");
    }
    expect(file_nonempty(cache / "index.sqlite"),
           "migrate: Store index at cache root");
    expect(file_nonempty(cache / "bulk.sqlite"),
           "migrate: Store bulk at cache root");
    // After move, store/ should not still hold the only copy of index.
    // Destination existed free → files renamed from store/ to top.
    expect(!file_nonempty(cache / "store" / "index.sqlite"),
           "migrate: store/ index moved away");

    // Store is usable after migrate (blob from seed should still resolve).
    auto& store = client->store();
    expect(store.index_schema_version() == thumtoo::kStoreIndexSchemaVersion,
           "post-migrate store schema");
  }

  // --- C: default is top-level Store ---
  {
    unsetenv("THUMTOO_STORE_ROOT");
    unsetenv("THUMTOO_STORE_ONLY");
    const fs::path cache = make_tmpdir("store-root-default");
    auto client = thumtoo::Client::open(cache, {}, 0, cache);
    expect(client != nullptr, "Client::open default");
    expect(file_nonempty(cache / "index.sqlite"),
           "default: Store index at cache root");
    expect(file_nonempty(cache / "bulk.sqlite"),
           "default: Store bulk at cache root");
    expect(!fs::exists(cache / "legacy"), "default: no legacy/ directory");
    expect(!fs::exists(cache / "store" / "index.sqlite"),
           "default: no store/ subdirectory");
  }

  
  if (g_failures) {
    std::cerr << g_failures << " failure(s)\n";
    return 1;
  }
  std::cout << "OK test_store_root\n";
  return 0;
}
