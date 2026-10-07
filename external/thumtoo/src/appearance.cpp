// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/appearance.hpp"
#include "thumtoo/constants.hpp"

#include <sqlite3.h>

#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace thumtoo {
namespace {

constexpr int kAppearanceSchemaVersion = 1;

std::int64_t unix_now()
{
  using clock = std::chrono::system_clock;
  return std::chrono::duration_cast<std::chrono::seconds>(
             clock::now().time_since_epoch())
      .count();
}

bool is_hex(char c)
{
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

}  // namespace

bool ContentAppearance::is_identity() const
{
  if (content_h_flip || content_v_flip || content_quarter_turns != 0) {
    return false;
  }
  if (has_crop && crop_w > 0 && crop_h > 0) {
    return false;
  }
  if (grade_brightness || grade_contrast || grade_saturation || grade_hue
      || grade_gamma || grade_invert) {
    return false;
  }
  return true;
}

std::filesystem::path default_state_root()
{
  if (const char* xdg = std::getenv("XDG_STATE_HOME"); xdg && xdg[0]) {
    return std::filesystem::path(xdg) / "thumtoo";
  }
  if (const char* home = std::getenv("HOME"); home && home[0]) {
    return std::filesystem::path(home) / ".local" / "state" / "thumtoo";
  }
  return std::filesystem::path("/tmp/thumtoo-state");
}

std::string normalize_content_id(std::string_view content_id_or_hex)
{
  std::string_view in = content_id_or_hex;
  if (in.starts_with(kContentIdSha256Prefix)) {
    in.remove_prefix(kContentIdSha256Prefix.size());
  }
  // File content: 64 hex digits.
  // Page variant: 64 hex + ":page:" + N
  // EPUB page: 64 hex + ":page:" + N + ":epub:" + layout_key
  //   (layout is part of tile identity; see docs/EPUB.md).
  std::string_view hex_part = in;
  std::string_view page_suffix;
  if (auto pos = in.find(":page:"); pos != std::string_view::npos) {
    hex_part = in.substr(0, pos);
    page_suffix = in.substr(pos); // ":page:N" or ":page:N:epub:…"
    if (page_suffix.size() < 7) {
      return {};
    }
    // ":page:" is 6 chars; digits for page number, then optional ":epub:…"
    std::size_t i = 6;
    while (i < page_suffix.size() && page_suffix[i] >= '0' && page_suffix[i] <= '9') {
      ++i;
    }
    if (i == 6) {
      return {};  // no digits
    }
    // reject page 0 and leading-zero forms like 01
    if (page_suffix[6] == '0' && i == 7) {
      return {};
    }
    if (i < page_suffix.size()) {
      constexpr std::string_view kEpub = ":epub:";
      if (page_suffix.size() < i + kEpub.size() ||
          page_suffix.substr(i, kEpub.size()) != kEpub ||
          page_suffix.size() == i + kEpub.size()) {
        return {};  // unknown suffix or empty layout key
      }
    }
  }
  if (hex_part.size() != 64) {
    return {};
  }
  std::string hex;
  hex.reserve(64);
  for (char c : hex_part) {
    if (!is_hex(c)) {
      return {};
    }
    hex.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
  }
  std::string out = std::string(kContentIdSha256Prefix) + hex;
  if (!page_suffix.empty()) {
    out.append(page_suffix.begin(), page_suffix.end());
  }
  return out;
}

AppearanceStore::AppearanceStore(void* db, std::filesystem::path db_path)
    : db_(db), db_path_(std::move(db_path))
{
}

AppearanceStore::AppearanceStore(AppearanceStore&& other) noexcept
    : db_(other.db_), db_path_(std::move(other.db_path_))
{
  other.db_ = nullptr;
}

AppearanceStore& AppearanceStore::operator=(AppearanceStore&& other) noexcept
{
  if (this != &other) {
    if (db_) {
      sqlite3_close(static_cast<sqlite3*>(db_));
    }
    db_ = other.db_;
    db_path_ = std::move(other.db_path_);
    other.db_ = nullptr;
  }
  return *this;
}

AppearanceStore::~AppearanceStore()
{
  if (db_) {
    sqlite3_close(static_cast<sqlite3*>(db_));
    db_ = nullptr;
  }
}

AppearanceStore AppearanceStore::open(const std::filesystem::path& state_root)
{
  std::error_code ec;
  std::filesystem::create_directories(state_root, ec);
  const auto path = state_root / "appearance.sqlite3";

  sqlite3* db = nullptr;
  if (sqlite3_open(path.string().c_str(), &db) != SQLITE_OK) {
    if (db) {
      sqlite3_close(db);
    }
    return AppearanceStore{};
  }
  auto try_exec = [&](const char* sql) -> bool {
    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
      sqlite3_free(err);
      return false;
    }
    return true;
  };
  if (!try_exec("PRAGMA journal_mode=WAL;") || !try_exec("PRAGMA busy_timeout=5000;")) {
    sqlite3_close(db);
    return AppearanceStore{};
  }
  if (!try_exec("CREATE TABLE IF NOT EXISTS schema_meta ("
                "  key TEXT PRIMARY KEY,"
                "  value TEXT NOT NULL"
                ");")) {
    sqlite3_close(db);
    return AppearanceStore{};
  }
  if (!try_exec("CREATE TABLE IF NOT EXISTS content_appearance ("
                "  content_id TEXT PRIMARY KEY,"
                "  version INTEGER NOT NULL DEFAULT 1,"
                "  updated_unix INTEGER NOT NULL,"
                "  content_h_flip INTEGER NOT NULL DEFAULT 0,"
                "  content_v_flip INTEGER NOT NULL DEFAULT 0,"
                "  content_quarter_turns INTEGER NOT NULL DEFAULT 0,"
                "  has_crop INTEGER NOT NULL DEFAULT 0,"
                "  crop_x INTEGER,"
                "  crop_y INTEGER,"
                "  crop_w INTEGER,"
                "  crop_h INTEGER,"
                "  crop_source_w INTEGER,"
                "  crop_source_h INTEGER,"
                "  crop_rotation REAL NOT NULL DEFAULT 0,"
                "  grade_brightness INTEGER,"
                "  grade_contrast INTEGER,"
                "  grade_saturation INTEGER,"
                "  grade_hue INTEGER,"
                "  grade_gamma INTEGER,"
                "  grade_invert INTEGER"
                ");")) {
    sqlite3_close(db);
    return AppearanceStore{};
  }
  // Additive migration for DBs created before grade_invert.
  try_exec("ALTER TABLE content_appearance ADD COLUMN grade_invert INTEGER;");

  // Record schema version (insert if missing).
  {
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(
            db, "INSERT OR IGNORE INTO schema_meta(key, value) VALUES('schema_version', ?);",
            -1, &st, nullptr)
        == SQLITE_OK) {
      const std::string ver = std::to_string(kAppearanceSchemaVersion);
      sqlite3_bind_text(st, 1, ver.c_str(), -1, SQLITE_TRANSIENT);
      sqlite3_step(st);
      sqlite3_finalize(st);
    }
  }

  return AppearanceStore(db, path);
}

std::optional<ContentAppearance> AppearanceStore::get(
    std::string_view content_id) const
{
  if (!db_) {
    return std::nullopt;
  }
  const std::string id = normalize_content_id(content_id);
  if (id.empty()) {
    return std::nullopt;
  }

  sqlite3_stmt* st = nullptr;
  const char* sql =
      "SELECT content_h_flip, content_v_flip, content_quarter_turns,"
      " has_crop, crop_x, crop_y, crop_w, crop_h,"
      " crop_source_w, crop_source_h, crop_rotation,"
      " grade_brightness, grade_contrast, grade_saturation,"
      " grade_hue, grade_gamma, grade_invert"
      " FROM content_appearance WHERE content_id = ?;";
  if (sqlite3_prepare_v2(static_cast<sqlite3*>(db_), sql, -1, &st, nullptr)
      != SQLITE_OK) {
    return std::nullopt;
  }
  sqlite3_bind_text(st, 1, id.c_str(), -1, SQLITE_TRANSIENT);
  std::optional<ContentAppearance> out;
  if (sqlite3_step(st) == SQLITE_ROW) {
    ContentAppearance a;
    a.content_h_flip = sqlite3_column_int(st, 0) != 0;
    a.content_v_flip = sqlite3_column_int(st, 1) != 0;
    a.content_quarter_turns = sqlite3_column_int(st, 2);
    a.has_crop = sqlite3_column_int(st, 3) != 0;
    if (sqlite3_column_type(st, 4) != SQLITE_NULL) {
      a.crop_x = sqlite3_column_int(st, 4);
      a.crop_y = sqlite3_column_int(st, 5);
      a.crop_w = sqlite3_column_int(st, 6);
      a.crop_h = sqlite3_column_int(st, 7);
    }
    if (sqlite3_column_type(st, 8) != SQLITE_NULL) {
      a.crop_source_w = sqlite3_column_int(st, 8);
      a.crop_source_h = sqlite3_column_int(st, 9);
    }
    a.crop_rotation = sqlite3_column_double(st, 10);
    auto opt_int = [&](int col) -> std::optional<int> {
      if (sqlite3_column_type(st, col) == SQLITE_NULL) {
        return std::nullopt;
      }
      return sqlite3_column_int(st, col);
    };
    a.grade_brightness = opt_int(11);
    a.grade_contrast = opt_int(12);
    a.grade_saturation = opt_int(13);
    a.grade_hue = opt_int(14);
    a.grade_gamma = opt_int(15);
    a.grade_invert = opt_int(16);
    out = a;
  }
  sqlite3_finalize(st);
  return out;
}

void AppearanceStore::put(std::string_view content_id, const ContentAppearance& app)
{
  if (!db_) {
    return;
  }
  const std::string id = normalize_content_id(content_id);
  if (id.empty()) {
    if (std::getenv("BILTOO_DEBUG_APPEARANCE") || std::getenv("THUMTOO_DEBUG_APPEARANCE")) {
      std::fprintf(stderr,
                   "[appearance] put SKIP: normalize_content_id rejected key (len=%zu)\n",
                   content_id.size());
    }
    return;
  }
  if (app.is_identity()) {
    remove(id);
    return;
  }

  ContentAppearance normalized = app;
  {
    int turns = normalized.content_quarter_turns % 4;
    if (turns < 0) {
      turns += 4;
    }
    normalized.content_quarter_turns = turns;
  }
  // Re-check after normalizing turns (e.g. 4 → 0).
  if (normalized.is_identity()) {
    remove(id);
    return;
  }

  const char* sql =
      "INSERT INTO content_appearance("
      " content_id, version, updated_unix,"
      " content_h_flip, content_v_flip, content_quarter_turns,"
      " has_crop, crop_x, crop_y, crop_w, crop_h,"
      " crop_source_w, crop_source_h, crop_rotation,"
      " grade_brightness, grade_contrast, grade_saturation,"
      " grade_hue, grade_gamma, grade_invert"
      ") VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)"
      " ON CONFLICT(content_id) DO UPDATE SET"
      " version=excluded.version,"
      " updated_unix=excluded.updated_unix,"
      " content_h_flip=excluded.content_h_flip,"
      " content_v_flip=excluded.content_v_flip,"
      " content_quarter_turns=excluded.content_quarter_turns,"
      " has_crop=excluded.has_crop,"
      " crop_x=excluded.crop_x,"
      " crop_y=excluded.crop_y,"
      " crop_w=excluded.crop_w,"
      " crop_h=excluded.crop_h,"
      " crop_source_w=excluded.crop_source_w,"
      " crop_source_h=excluded.crop_source_h,"
      " crop_rotation=excluded.crop_rotation,"
      " grade_brightness=excluded.grade_brightness,"
      " grade_contrast=excluded.grade_contrast,"
      " grade_saturation=excluded.grade_saturation,"
      " grade_hue=excluded.grade_hue,"
      " grade_gamma=excluded.grade_gamma,"
      " grade_invert=excluded.grade_invert;";

  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(static_cast<sqlite3*>(db_), sql, -1, &st, nullptr)
      != SQLITE_OK) {
    return;
  }
  int i = 1;
  sqlite3_bind_text(st, i++, id.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(st, i++, kAppearanceSchemaVersion);
  sqlite3_bind_int64(st, i++, unix_now());
  sqlite3_bind_int(st, i++, normalized.content_h_flip ? 1 : 0);
  sqlite3_bind_int(st, i++, normalized.content_v_flip ? 1 : 0);
  sqlite3_bind_int(st, i++, normalized.content_quarter_turns);
  sqlite3_bind_int(st, i++, normalized.has_crop ? 1 : 0);
  if (normalized.has_crop) {
    sqlite3_bind_int(st, i++, normalized.crop_x);
    sqlite3_bind_int(st, i++, normalized.crop_y);
    sqlite3_bind_int(st, i++, normalized.crop_w);
    sqlite3_bind_int(st, i++, normalized.crop_h);
    sqlite3_bind_int(st, i++, normalized.crop_source_w);
    sqlite3_bind_int(st, i++, normalized.crop_source_h);
  } else {
    for (int n = 0; n < 6; ++n) {
      sqlite3_bind_null(st, i++);
    }
  }
  sqlite3_bind_double(st, i++, normalized.crop_rotation);
  auto bind_opt = [&](const std::optional<int>& v) {
    if (v) {
      sqlite3_bind_int(st, i++, *v);
    } else {
      sqlite3_bind_null(st, i++);
    }
  };
  bind_opt(normalized.grade_brightness);
  bind_opt(normalized.grade_contrast);
  bind_opt(normalized.grade_saturation);
  bind_opt(normalized.grade_hue);
  bind_opt(normalized.grade_gamma);
  bind_opt(normalized.grade_invert);
  const int rc = sqlite3_step(st);
  sqlite3_finalize(st);
  if (rc != SQLITE_DONE) {
    // Leave the store open; caller treats missing rows as identity.
    return;
  }
}

void AppearanceStore::remove(std::string_view content_id)
{
  if (!db_) {
    return;
  }
  const std::string id = normalize_content_id(content_id);
  if (id.empty()) {
    return;
  }
  sqlite3_stmt* st = nullptr;
  if (sqlite3_prepare_v2(static_cast<sqlite3*>(db_),
                         "DELETE FROM content_appearance WHERE content_id = ?;",
                         -1, &st, nullptr)
      != SQLITE_OK) {
    return;
  }
  sqlite3_bind_text(st, 1, id.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_step(st);
  sqlite3_finalize(st);
}

}  // namespace thumtoo
