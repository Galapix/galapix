// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/layout.hpp"
#include "thumtoo/constants.hpp"

#include "sqlite3.h"

#include <optional>
#include <cstdlib>
#include <string>

namespace thumtoo {
namespace {

std::optional<int> probe_index_schema_version(
    const std::filesystem::path& index_path) {
  namespace fs = std::filesystem;
  std::error_code ec;
  if (!fs::is_regular_file(index_path, ec)) return std::nullopt;
  sqlite3* db = nullptr;
  if (sqlite3_open_v2(index_path.string().c_str(), &db, SQLITE_OPEN_READONLY,
                      nullptr) != SQLITE_OK) {
    if (db) sqlite3_close(db);
    return std::nullopt;
  }
  sqlite3_stmt* stmt = nullptr;
  const char* sql = "SELECT value FROM schema_meta WHERE key = ?1;";
  std::optional<int> out;
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_text(stmt, 1, kSchemaMetaVersionKey, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) == SQLITE_ROW &&
        sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
      try {
        out = std::stoi(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
      } catch (...) {
      }
    }
    sqlite3_finalize(stmt);
  }
  sqlite3_close(db);
  return out;
}

/// Move stem (+ optional -wal/-shm) from from_base to to_base. No-op if source
/// is missing or destination already exists. Falls back to copy+remove when
/// rename fails (cross-device, some sandbox layouts).
void rename_sqlite_bundle(const std::filesystem::path& from_base,
                          const std::filesystem::path& to_base,
                          const char* stem) {
  namespace fs = std::filesystem;
  std::error_code ec;
  const fs::path from = from_base / stem;
  const fs::path to = to_base / stem;
  if (!fs::is_regular_file(from, ec)) return;
  if (fs::exists(to, ec)) return;
  fs::create_directories(to_base, ec);
  ec.clear();
  fs::rename(from, to, ec);
  if (ec) {
    ec.clear();
    fs::copy_file(from, to, fs::copy_options::none, ec);
    if (!ec) {
      fs::remove(from, ec);
    }
  }
  for (const char* suf : {"-wal", "-shm", "-journal"}) {
    const fs::path f2 = fs::path(from.string() + suf);
    const fs::path t2 = fs::path(to.string() + suf);
    if (!fs::exists(f2, ec) || fs::exists(t2, ec)) continue;
    ec.clear();
    fs::rename(f2, t2, ec);
    if (ec) {
      ec.clear();
      fs::copy_file(f2, t2, fs::copy_options::none, ec);
      if (!ec) fs::remove(f2, ec);
    }
  }
}

}  // namespace

std::filesystem::path legacy_db_root(const std::filesystem::path& cache_root) {
  return cache_root / "legacy";
}

std::filesystem::path redesign_store_root(
    const std::filesystem::path& cache_root) {
  return cache_root;
}

void migrate_dual_path_to_store_root(const std::filesystem::path& cache_root) {
  namespace fs = std::filesystem;
  const fs::path top_index = cache_root / "index.sqlite";
  const fs::path top_blobs = cache_root / "blobs.sqlite";
  const fs::path store_dir = cache_root / "store";
  const fs::path store_index = store_dir / "index.sqlite";
  const fs::path legacy_dir = cache_root / "legacy";
  const fs::path legacy_index = legacy_dir / "index.sqlite";
  const fs::path legacy_blobs = legacy_dir / "blobs.sqlite";

  auto top_ver = probe_index_schema_version(top_index);
  auto store_ver = probe_index_schema_version(store_index);
  auto legacy_ver = probe_index_schema_version(legacy_index);

  // Classic dual-path (schema < 100) at cache root → park under legacy/.
  // Move index and blobs together so a partial rename cannot leave blobs at top.
  if (top_ver && *top_ver < 100 && !legacy_ver) {
    rename_sqlite_bundle(cache_root, legacy_dir, "index.sqlite");
    rename_sqlite_bundle(cache_root, legacy_dir, "blobs.sqlite");
  }

  // Recovery: legacy index already present but classic blobs still at top
  // (interrupted migrate or earlier tip that only moved index).
  {
    std::error_code ec;
    if (fs::is_regular_file(legacy_index, ec) &&
        fs::is_regular_file(top_blobs, ec) &&
        !fs::exists(legacy_blobs, ec)) {
      rename_sqlite_bundle(cache_root, legacy_dir, "blobs.sqlite");
    }
  }

  top_ver = probe_index_schema_version(top_index);
  store_ver = probe_index_schema_version(store_index);
  if (store_ver && *store_ver >= 100 && !top_ver) {
    rename_sqlite_bundle(store_dir, cache_root, "index.sqlite");
    rename_sqlite_bundle(store_dir, cache_root, "bulk.sqlite");
  }
}


std::filesystem::path default_data_root() {
  if (const char* xdg = std::getenv("XDG_STATE_HOME"); xdg && xdg[0]) {
    return std::filesystem::path(xdg) / "thumtoo";
  }
  if (const char* home = std::getenv("HOME"); home && home[0]) {
    return std::filesystem::path(home) / ".local" / "state" / "thumtoo";
  }
  return std::filesystem::path("/tmp/thumtoo-state");
}

namespace {

std::filesystem::path xdg_data_home_thumtoo() {
  if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && xdg[0]) {
    return std::filesystem::path(xdg) / "thumtoo";
  }
  if (const char* home = std::getenv("HOME"); home && home[0]) {
    return std::filesystem::path(home) / ".local" / "share" / "thumtoo";
  }
  return {};
}

bool try_relocate_user_sqlite(const std::filesystem::path& from,
                              const std::filesystem::path& to_dir) {
  namespace fs = std::filesystem;
  std::error_code ec;
  if (!fs::is_regular_file(from, ec)) {
    return false;
  }
  fs::create_directories(to_dir, ec);
  if (ec) {
    return false;
  }
  const fs::path dest = to_dir / "user.sqlite";
  if (fs::exists(dest, ec)) {
    return false;
  }
  ec.clear();
  fs::rename(from, dest, ec);
  if (!ec) {
    return true;
  }
  ec.clear();
  fs::copy_file(from, dest, fs::copy_options::none, ec);
  if (!ec) {
    fs::remove(from, ec);
    return true;
  }
  return false;
}

}  // namespace

void migrate_user_sqlite_to_data_root(const std::filesystem::path& cache_root,
                                      const std::filesystem::path& data_root) {
  namespace fs = std::filesystem;
  if (data_root.empty()) {
    return;
  }
  std::error_code ec;
  const fs::path dest = data_root / "user.sqlite";
  if (fs::is_regular_file(dest, ec)) {
    return;
  }
  // Prefer cache_root (old default when data_root == cache).
  if (!cache_root.empty()
      && try_relocate_user_sqlite(cache_root / "user.sqlite", data_root)) {
    return;
  }
  // Pre-STATE default was XDG_DATA_HOME/thumtoo.
  const fs::path legacy = xdg_data_home_thumtoo();
  if (!legacy.empty() && legacy != data_root) {
    (void)try_relocate_user_sqlite(legacy / "user.sqlite", data_root);
  }
}

}  // namespace thumtoo
