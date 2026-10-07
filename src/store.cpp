// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/store.hpp"
#include "thumtoo/constants.hpp"

#include "sqlite3.h"

#include <chrono>
#include <cctype>
#include <cstring>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace thumtoo {
namespace {

std::int64_t now_unix_s() {
  using namespace std::chrono;
  return duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
}

[[noreturn]] void throw_sqlite(sqlite3* db, const char* what) {
  const char* msg = db ? sqlite3_errmsg(db) : "null db";
  throw std::runtime_error(std::string(what) + ": " + (msg ? msg : ""));
}

void exec_sql(sqlite3* db, const char* sql) {
  char* err = nullptr;
  if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
    std::string msg = err ? err : "sqlite3_exec failed";
    sqlite3_free(err);
    throw std::runtime_error(msg);
  }
}

std::optional<std::string> meta_get_db(sqlite3* db, std::string_view key) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, "SELECT value FROM schema_meta WHERE key = ?1;", -1,
                         &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(db, "prepare meta_get");
  }
  sqlite3_bind_text(stmt, 1, key.data(), static_cast<int>(key.size()),
                    SQLITE_STATIC);
  std::optional<std::string> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const char* v =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (v) out = v;
  }
  sqlite3_finalize(stmt);
  return out;
}

void meta_set_db(sqlite3* db, std::string_view key, std::string_view value) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db,
                         "INSERT INTO schema_meta(key, value) VALUES(?1, ?2) "
                         "ON CONFLICT(key) DO UPDATE SET value = excluded.value;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(db, "prepare meta_set");
  }
  sqlite3_bind_text(stmt, 1, key.data(), static_cast<int>(key.size()),
                    SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, value.data(), static_cast<int>(value.size()),
                    SQLITE_STATIC);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(db, "step meta_set");
  }
  sqlite3_finalize(stmt);
}

int hex_nibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

constexpr char kIndexSchemaSql[] = R"SQL(
CREATE TABLE IF NOT EXISTS schema_meta (
  key   TEXT PRIMARY KEY,
  value TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS blob (
  id         INTEGER PRIMARY KEY,
  size       INTEGER,
  status     INTEGER NOT NULL DEFAULT 0,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL
);
CREATE TABLE IF NOT EXISTS hash_algo (
  id   INTEGER PRIMARY KEY,
  name TEXT NOT NULL UNIQUE
);
CREATE TABLE IF NOT EXISTS blob_hash (
  blob_id INTEGER NOT NULL REFERENCES blob(id) ON DELETE CASCADE,
  algo_id INTEGER NOT NULL REFERENCES hash_algo(id),
  digest  BLOB NOT NULL,
  PRIMARY KEY (blob_id, algo_id),
  UNIQUE (algo_id, digest)
);
CREATE INDEX IF NOT EXISTS idx_blob_hash_digest ON blob_hash(algo_id, digest);
CREATE TABLE IF NOT EXISTS locator (
  id          INTEGER PRIMARY KEY,
  uri         TEXT NOT NULL UNIQUE,
  blob_id     INTEGER REFERENCES blob(id) ON DELETE SET NULL,
  size        INTEGER,
  mtime_ns    INTEGER,
  outer_path  TEXT,
  member_path TEXT,
  updated_at  INTEGER NOT NULL
);
-- idx_locator_outer_path is created in ensure_optional_index_tables after
-- outer_path is guaranteed present (pre-284 schema-100 indexes lack the column).
CREATE INDEX IF NOT EXISTS idx_locator_blob ON locator(blob_id);
CREATE TABLE IF NOT EXISTS codec (
  id   INTEGER PRIMARY KEY,
  name TEXT NOT NULL UNIQUE
);
CREATE TABLE IF NOT EXISTS container_member (
  container_id       INTEGER NOT NULL REFERENCES blob(id) ON DELETE CASCADE,
  member_path        TEXT NOT NULL,
  is_directory       INTEGER NOT NULL DEFAULT 0,
  uncompressed_size  INTEGER,
  blob_id            INTEGER REFERENCES blob(id) ON DELETE SET NULL,
  PRIMARY KEY (container_id, member_path)
);
CREATE INDEX IF NOT EXISTS idx_container_member_blob ON container_member(blob_id);
CREATE TABLE IF NOT EXISTS media (
  id           INTEGER PRIMARY KEY,
  blob_id      INTEGER NOT NULL REFERENCES blob(id) ON DELETE CASCADE,
  kind         INTEGER NOT NULL DEFAULT 0,
  width        INTEGER,
  height       INTEGER,
  duration_ms  INTEGER,
  page_count   INTEGER,
  still_count  INTEGER,
  status       INTEGER NOT NULL DEFAULT 0,
  error_code   TEXT,
  updated_at   INTEGER NOT NULL,
  UNIQUE (blob_id, kind)
);
CREATE INDEX IF NOT EXISTS idx_media_blob ON media(blob_id);
CREATE TABLE IF NOT EXISTS blob_lqip (
  blob_id INTEGER NOT NULL REFERENCES blob(id) ON DELETE CASCADE,
  page_1based INTEGER NOT NULL DEFAULT 0,
  kind    INTEGER NOT NULL DEFAULT 0,
  data    BLOB NOT NULL,
  PRIMARY KEY (blob_id, page_1based)
);
CREATE TABLE IF NOT EXISTS page_text_layer (
  blob_id INTEGER NOT NULL REFERENCES blob(id) ON DELETE CASCADE,
  page_1based INTEGER NOT NULL,
  layout_key TEXT NOT NULL DEFAULT '',
  data BLOB NOT NULL,
  PRIMARY KEY (blob_id, page_1based, layout_key)
);
CREATE TABLE IF NOT EXISTS document_outline (
  blob_id INTEGER NOT NULL REFERENCES blob(id) ON DELETE CASCADE,
  layout_key TEXT NOT NULL DEFAULT '',
  data BLOB NOT NULL,
  PRIMARY KEY (blob_id, layout_key)
);

CREATE TABLE IF NOT EXISTS region (
  id         INTEGER PRIMARY KEY,
  media_id   INTEGER NOT NULL REFERENCES media(id) ON DELETE CASCADE,
  kind       INTEGER NOT NULL,
  key        TEXT NOT NULL,
  ordinal    INTEGER,
  width      INTEGER,
  height     INTEGER,
  UNIQUE (media_id, kind, key)
);
CREATE INDEX IF NOT EXISTS idx_region_media ON region(media_id);
CREATE TABLE IF NOT EXISTS region_key (
  region_id INTEGER NOT NULL REFERENCES region(id) ON DELETE CASCADE,
  key_kind  INTEGER NOT NULL,
  key       TEXT NOT NULL,
  PRIMARY KEY (region_id, key_kind)
);
CREATE TABLE IF NOT EXISTS tile (
  media_id   INTEGER NOT NULL REFERENCES media(id) ON DELETE CASCADE,
  region_id  INTEGER NOT NULL REFERENCES region(id) ON DELETE CASCADE,
  scale      INTEGER NOT NULL,
  x          INTEGER NOT NULL,
  y          INTEGER NOT NULL,
  width      INTEGER NOT NULL,
  height     INTEGER NOT NULL,
  codec_id   INTEGER NOT NULL REFERENCES codec(id),
  quality    INTEGER,
  PRIMARY KEY (media_id, region_id, scale, x, y)
);
CREATE TABLE IF NOT EXISTS directory_snapshot (
  dir_uri    TEXT PRIMARY KEY,
  size       INTEGER,
  mtime_ns   INTEGER,
  listed_at  INTEGER NOT NULL,
  incomplete INTEGER NOT NULL DEFAULT 0,
  inode      INTEGER,
  dev        INTEGER,
  nlink      INTEGER,
  mode       INTEGER,
  uid        INTEGER,
  gid        INTEGER,
  error_code TEXT
);
CREATE TABLE IF NOT EXISTS directory_entry (
  dir_uri         TEXT NOT NULL REFERENCES directory_snapshot(dir_uri) ON DELETE CASCADE,
  name            TEXT NOT NULL,
  child_uri       TEXT,
  is_dir          INTEGER NOT NULL,
  size            INTEGER,
  mtime_ns        INTEGER,
  file_type       INTEGER,
  mode            INTEGER,
  uid             INTEGER,
  gid             INTEGER,
  atime_ns        INTEGER,
  ctime_ns        INTEGER,
  birth_ns        INTEGER,
  nlink           INTEGER,
  inode           INTEGER,
  dev             INTEGER,
  rdev            INTEGER,
  symlink_target  TEXT,
  blob_id         INTEGER,
  flags           INTEGER NOT NULL DEFAULT 0,
  PRIMARY KEY (dir_uri, name)
);
CREATE TABLE IF NOT EXISTS entry_xattr (
  dir_uri  TEXT NOT NULL,
  name     TEXT NOT NULL,
  key      TEXT NOT NULL,
  value    BLOB,
  PRIMARY KEY (dir_uri, name, key)
);
)SQL";

constexpr char kBulkSchemaSql[] = R"SQL(
CREATE TABLE IF NOT EXISTS schema_meta (
  key   TEXT PRIMARY KEY,
  value TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS tile_blob (
  media_id   INTEGER NOT NULL,
  region_id  INTEGER NOT NULL,
  scale      INTEGER NOT NULL,
  x          INTEGER NOT NULL,
  y          INTEGER NOT NULL,
  codec_id   INTEGER NOT NULL,
  data       BLOB NOT NULL,
  PRIMARY KEY (media_id, region_id, scale, x, y)
);
CREATE TABLE IF NOT EXISTS http_body (
  url         TEXT PRIMARY KEY,
  fetched_at  INTEGER NOT NULL,
  blob_id     INTEGER,
  data        BLOB NOT NULL
);
)SQL";

constexpr char kUserSchemaSql[] = R"SQL(
CREATE TABLE IF NOT EXISTS schema_meta (
  key   TEXT PRIMARY KEY,
  value TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS tag_def (
  id         INTEGER PRIMARY KEY,
  uuid       TEXT UNIQUE,
  name       TEXT NOT NULL UNIQUE,
  label      TEXT,
  color      TEXT,
  badge      TEXT,
  created_at INTEGER NOT NULL
);
CREATE TABLE IF NOT EXISTS blob_tag (
  blob_ref   TEXT NOT NULL,
  tag_id     INTEGER NOT NULL REFERENCES tag_def(id) ON DELETE CASCADE,
  tagged_at  INTEGER NOT NULL,
  source     TEXT,
  PRIMARY KEY (blob_ref, tag_id)
);
CREATE INDEX IF NOT EXISTS idx_blob_tag_tag ON blob_tag(tag_id);
CREATE TABLE IF NOT EXISTS collection (
  id         INTEGER PRIMARY KEY,
  uuid       TEXT UNIQUE,
  label      TEXT,
  color      TEXT,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL
);
CREATE TABLE IF NOT EXISTS collection_member (
  collection_id INTEGER NOT NULL REFERENCES collection(id) ON DELETE CASCADE,
  blob_ref      TEXT NOT NULL,
  ordinal       INTEGER,
  path_key      TEXT,
  PRIMARY KEY (collection_id, blob_ref)
);
CREATE TABLE IF NOT EXISTS bookmark (
  id         INTEGER PRIMARY KEY,
  target_ref TEXT NOT NULL,
  title      TEXT,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_bookmark_target ON bookmark(target_ref);
CREATE TABLE IF NOT EXISTS link_edge (
  id         INTEGER PRIMARY KEY,
  from_ref   TEXT NOT NULL,
  to_ref     TEXT NOT NULL,
  rel        TEXT,
  source     INTEGER NOT NULL DEFAULT 2,
  created_at INTEGER NOT NULL,
  UNIQUE (from_ref, to_ref, rel, source)
);
CREATE INDEX IF NOT EXISTS idx_user_link_from ON link_edge(from_ref);
CREATE INDEX IF NOT EXISTS idx_user_link_to ON link_edge(to_ref);
CREATE TABLE IF NOT EXISTS annotation (
  id         INTEGER PRIMARY KEY,
  target_ref TEXT NOT NULL,
  kind       INTEGER NOT NULL,
  body       TEXT,
  geom       BLOB,
  created_at INTEGER NOT NULL,
  updated_at INTEGER NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_annotation_target ON annotation(target_ref);
)SQL";

}  // namespace

Store::Store(sqlite3* index, sqlite3* bulk, sqlite3* user,
             std::filesystem::path cache_root, std::filesystem::path data_root,
             int index_schema_version)
    : index_(index),
      bulk_(bulk),
      user_(user),
      cache_root_(std::move(cache_root)),
      data_root_(std::move(data_root)),
      index_schema_version_(index_schema_version) {}

Store::Store(Store&& other) noexcept
    : index_(std::exchange(other.index_, nullptr)),
      bulk_(std::exchange(other.bulk_, nullptr)),
      user_(std::exchange(other.user_, nullptr)),
      cache_root_(std::move(other.cache_root_)),
      data_root_(std::move(other.data_root_)),
      index_schema_version_(other.index_schema_version_) {}

Store& Store::operator=(Store&& other) noexcept {
  if (this != &other) {
    close_sqlite(index_);
    close_sqlite(bulk_);
    close_sqlite(user_);
    index_ = std::exchange(other.index_, nullptr);
    bulk_ = std::exchange(other.bulk_, nullptr);
    user_ = std::exchange(other.user_, nullptr);
    cache_root_ = std::move(other.cache_root_);
    data_root_ = std::move(other.data_root_);
    index_schema_version_ = other.index_schema_version_;
  }
  return *this;
}

Store::~Store() {
  close_sqlite(index_);
  close_sqlite(bulk_);
  close_sqlite(user_);
}

void Store::close_sqlite(sqlite3*& db) {
  if (db) {
    sqlite3_close(db);
    db = nullptr;
  }
}

sqlite3* Store::open_sqlite(const std::filesystem::path& path) {
  sqlite3* db = nullptr;
  if (sqlite3_open(path.string().c_str(), &db) != SQLITE_OK) {
    const char* msg = db ? sqlite3_errmsg(db) : "open failed";
    std::string err = path.string() + ": " + (msg ? msg : "");
    if (db) sqlite3_close(db);
    throw std::runtime_error(err);
  }
  exec_sql(db, "PRAGMA journal_mode=WAL;");
  exec_sql(db, "PRAGMA busy_timeout=5000;");
  exec_sql(db, "PRAGMA foreign_keys=ON;");
  return db;
}

namespace {

sqlite3* open_sqlite_memory() {
  sqlite3* db = nullptr;
  // Each open is an independent private in-memory database.
  if (sqlite3_open(":memory:", &db) != SQLITE_OK) {
    const char* msg = db ? sqlite3_errmsg(db) : "open failed";
    std::string err = std::string(":memory: ") + (msg ? msg : "");
    if (db) sqlite3_close(db);
    throw std::runtime_error(err);
  }
  // WAL is for on-disk files; MEMORY journal fits ephemeral DBs.
  exec_sql(db, "PRAGMA journal_mode=MEMORY;");
  exec_sql(db, "PRAGMA foreign_keys=ON;");
  return db;
}

}  // namespace

Store Store::open_memory() {
  sqlite3* index = open_sqlite_memory();
  sqlite3* bulk = open_sqlite_memory();
  sqlite3* user = open_sqlite_memory();
  Store store(index, bulk, user, /*cache_root=*/{}, /*data_root=*/{}, 0);
  store.migrate_or_init_index();
  store.ensure_optional_index_tables();
  store.migrate_or_init_bulk();
  store.migrate_or_init_user();
  return store;
}

void Store::exec_index(const char* sql) const { exec_sql(index_, sql); }
void Store::exec_bulk(const char* sql) const { exec_sql(bulk_, sql); }
void Store::exec_user(const char* sql) const { exec_sql(user_, sql); }

void Store::seed_lookups() {
  sqlite3_stmt* stmt = nullptr;
  const char* sql =
      "INSERT OR IGNORE INTO hash_algo(id, name) VALUES(?1, ?2);";
  if (sqlite3_prepare_v2(index_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare hash_algo seed");
  }
  auto bind_algo = [&](int id, const char* name) {
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
      sqlite3_finalize(stmt);
      throw_sqlite(index_, "seed hash_algo");
    }
  };
  bind_algo(static_cast<int>(HashAlgoId::Sha256), "sha256");
  bind_algo(static_cast<int>(HashAlgoId::Sha1), "sha1");
  bind_algo(static_cast<int>(HashAlgoId::Md5), "md5");
  bind_algo(static_cast<int>(HashAlgoId::Crc32), "crc32");
  sqlite3_finalize(stmt);

  if (sqlite3_prepare_v2(index_,
                         "INSERT OR IGNORE INTO codec(id, name) VALUES(?1, ?2);",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare codec seed");
  }
  auto bind_codec = [&](int id, const char* name) {
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_text(stmt, 2, name, -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
      sqlite3_finalize(stmt);
      throw_sqlite(index_, "seed codec");
    }
  };
  bind_codec(static_cast<int>(CodecId::Jpeg), "jpeg");
  bind_codec(static_cast<int>(CodecId::Jxl), "jxl");
  bind_codec(static_cast<int>(CodecId::Png), "png");
  sqlite3_finalize(stmt);
}

namespace {

bool index_table_has_column(sqlite3* db, const char* table, const char* column) {
  if (!db || !table || !column) return false;
  // PRAGMA table_info rows: cid, name, type, notnull, dflt_value, pk
  std::string sql = std::string("PRAGMA table_info(") + table + ");";
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
    return false;
  }
  bool found = false;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char* name =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (name && std::strcmp(name, column) == 0) {
      found = true;
      break;
    }
  }
  sqlite3_finalize(stmt);
  return found;
}

void ensure_locator_column(sqlite3* db, const char* column, const char* sql_type) {
  if (index_table_has_column(db, "locator", column)) return;
  std::string alter =
      std::string("ALTER TABLE locator ADD COLUMN ") + column + " " + sql_type + ";";
  char* err = nullptr;
  const int rc = sqlite3_exec(db, alter.c_str(), nullptr, nullptr, &err);
  if (rc != SQLITE_OK) {
    std::string msg = err ? err : "ALTER TABLE locator failed";
    sqlite3_free(err);
    // Duplicate column is fine (race / concurrent open); re-check below.
    if (msg.find("duplicate column") == std::string::npos) {
      throw std::runtime_error(msg);
    }
  }
  if (!index_table_has_column(db, "locator", column)) {
    throw std::runtime_error(
        std::string("locator.") + column +
        " missing after ALTER; delete cache index.sqlite and retry");
  }
}


void ensure_table_column(sqlite3* db, const char* table, const char* column,
                         const char* sql_type) {
  if (index_table_has_column(db, table, column)) return;
  std::string alter = std::string("ALTER TABLE ") + table + " ADD COLUMN " +
                      column + " " + sql_type + ";";
  char* err = nullptr;
  const int rc = sqlite3_exec(db, alter.c_str(), nullptr, nullptr, &err);
  if (rc != SQLITE_OK) {
    std::string msg = err ? err : "ALTER TABLE failed";
    sqlite3_free(err);
    if (msg.find("duplicate column") == std::string::npos) {
      throw std::runtime_error(msg);
    }
  }
  if (!index_table_has_column(db, table, column)) {
    throw std::runtime_error(std::string(table) + "." + column +
                             " missing after ALTER");
  }
}

}  // namespace

void Store::ensure_optional_index_tables() {
  // outer_path/member_path were added after the first redesign cutover.
  ensure_locator_column(index_, "outer_path", "TEXT");
  ensure_locator_column(index_, "member_path", "TEXT");
  exec_index(
      "CREATE INDEX IF NOT EXISTS idx_locator_outer_path ON "
      "locator(outer_path);");
  // Schema ≥101 directory POSIX columns (additive on existing 100 DBs).
  for (const char* col : {"inode", "dev", "nlink", "mode", "uid", "gid"}) {
    ensure_table_column(index_, "directory_snapshot", col, "INTEGER");
  }
  ensure_table_column(index_, "directory_snapshot", "error_code", "TEXT");
  for (const char* col :
       {"file_type", "mode", "uid", "gid", "atime_ns", "ctime_ns", "birth_ns",
        "nlink", "inode", "dev", "rdev", "blob_id", "flags"}) {
    ensure_table_column(index_, "directory_entry", col, "INTEGER");
  }
  ensure_table_column(index_, "directory_entry", "symlink_target", "TEXT");
  exec_index(
      "CREATE TABLE IF NOT EXISTS entry_xattr ("
      "  dir_uri  TEXT NOT NULL,"
      "  name     TEXT NOT NULL,"
      "  key      TEXT NOT NULL,"
      "  value    BLOB,"
      "  PRIMARY KEY (dir_uri, name, key)"
      ");");
  // Prefer composite-key blob_lqip. If a legacy single-column table exists
  // (blob_id PK only), rebuild it and copy page-0 rows.
  {
    sqlite3_stmt* st = nullptr;
    const bool has_page =
        sqlite3_prepare_v2(index_, "SELECT page_1based FROM blob_lqip LIMIT 0;",
                           -1, &st, nullptr) == SQLITE_OK;
    if (st) sqlite3_finalize(st);
    if (!has_page) {
      // Table missing or legacy shape — migrate via rename when present.
      sqlite3_exec(index_, "ALTER TABLE blob_lqip RENAME TO blob_lqip_legacy;",
                   nullptr, nullptr, nullptr);
      exec_index(
          "CREATE TABLE IF NOT EXISTS blob_lqip ("
          "  blob_id INTEGER NOT NULL REFERENCES blob(id) ON DELETE CASCADE,"
          "  page_1based INTEGER NOT NULL DEFAULT 0,"
          "  kind INTEGER NOT NULL DEFAULT 0,"
          "  data BLOB NOT NULL,"
          "  PRIMARY KEY (blob_id, page_1based)"
          ");");
      sqlite3_exec(
          index_,
          "INSERT OR IGNORE INTO blob_lqip(blob_id, page_1based, kind, data) "
          "SELECT blob_id, 0, kind, data FROM blob_lqip_legacy;",
          nullptr, nullptr, nullptr);
      sqlite3_exec(index_, "DROP TABLE IF EXISTS blob_lqip_legacy;", nullptr,
                   nullptr, nullptr);
    } else {
      exec_index(
          "CREATE TABLE IF NOT EXISTS blob_lqip ("
          "  blob_id INTEGER NOT NULL REFERENCES blob(id) ON DELETE CASCADE,"
          "  page_1based INTEGER NOT NULL DEFAULT 0,"
          "  kind INTEGER NOT NULL DEFAULT 0,"
          "  data BLOB NOT NULL,"
          "  PRIMARY KEY (blob_id, page_1based)"
          ");");
    }
  }
  exec_index(
      "CREATE TABLE IF NOT EXISTS page_text_layer ("
      "  blob_id INTEGER NOT NULL REFERENCES blob(id) ON DELETE CASCADE,"
      "  page_1based INTEGER NOT NULL,"
      "  layout_key TEXT NOT NULL DEFAULT '',"
      "  data BLOB NOT NULL,"
      "  PRIMARY KEY (blob_id, page_1based, layout_key)"
      ");");
  exec_index(
      "CREATE TABLE IF NOT EXISTS document_outline ("
      "  blob_id INTEGER NOT NULL REFERENCES blob(id) ON DELETE CASCADE,"
      "  layout_key TEXT NOT NULL DEFAULT '',"
      "  data BLOB NOT NULL,"
      "  PRIMARY KEY (blob_id, layout_key)"
      ");");

  // Per-page document layout size (not shared media dims).
  ensure_table_column(index_, "region", "width", "INTEGER");
  ensure_table_column(index_, "region", "height", "INTEGER");
}

void Store::migrate_or_init_index() {
  exec_index(kIndexSchemaSql);
  auto ver = meta_get_db(index_, kSchemaMetaVersionKey);
  if (!ver) {
    meta_set_db(index_, kSchemaMetaVersionKey,
                std::to_string(kStoreIndexSchemaVersion));
    index_schema_version_ = kStoreIndexSchemaVersion;
    seed_lookups();
    return;
  }
  index_schema_version_ = std::stoi(*ver);
  if (index_schema_version_ > kStoreIndexSchemaVersion) {
    throw std::runtime_error(
        "index schema_version is newer than this build");
  }
  if (index_schema_version_ < kStoreIndexSchemaVersion) {
    if (index_schema_version_ < 100) {
      // Pre-redesign / ladder epoch: still no automated conversion.
      throw std::runtime_error(
          "index schema_version is outdated; delete cache index.sqlite and "
          "retry");
    }
    // Additive path: 100 → 101 (directory POSIX columns + entry_xattr).
    // ensure_optional_index_tables() runs after this and applies ALTERs.
    meta_set_db(index_, kSchemaMetaVersionKey,
                std::to_string(kStoreIndexSchemaVersion));
    index_schema_version_ = kStoreIndexSchemaVersion;
  }
  seed_lookups();
}

void Store::migrate_or_init_bulk() {
  exec_bulk(kBulkSchemaSql);
  auto ver = meta_get_db(bulk_, kSchemaMetaVersionKey);
  if (!ver) {
    meta_set_db(bulk_, kSchemaMetaVersionKey,
                std::to_string(kStoreBulkSchemaVersion));
    return;
  }
  const int v = std::stoi(*ver);
  if (v > kStoreBulkSchemaVersion) {
    throw std::runtime_error("bulk schema_version is newer than this build");
  }
  if (v < kStoreBulkSchemaVersion) {
    throw std::runtime_error(
        "bulk schema_version is outdated; delete cache bulk.sqlite and retry");
  }
}

void Store::migrate_or_init_user() {
  exec_user(kUserSchemaSql);
  auto ver = meta_get_db(user_, kSchemaMetaVersionKey);
  if (!ver) {
    meta_set_db(user_, kSchemaMetaVersionKey,
                std::to_string(kStoreUserSchemaVersion));
    return;
  }
  const int v = std::stoi(*ver);
  if (v > kStoreUserSchemaVersion) {
    throw std::runtime_error("user schema_version is newer than this build");
  }
  if (v < kStoreUserSchemaVersion) {
    if (v < 100) {
      throw std::runtime_error(
          "user schema_version is outdated; migrate or replace user.sqlite");
    }
    // Additive 100 → 101: stable uuid keys for tag_def / collection.
    ensure_table_column(user_, "tag_def", "uuid", "TEXT");
    ensure_table_column(user_, "collection", "uuid", "TEXT");
    // UNIQUE indexes (SQLite allows multiple NULLs).
    exec_user(
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_tag_def_uuid ON "
        "tag_def(uuid) WHERE uuid IS NOT NULL;");
    exec_user(
        "CREATE UNIQUE INDEX IF NOT EXISTS idx_collection_uuid ON "
        "collection(uuid) WHERE uuid IS NOT NULL;");
    meta_set_db(user_, kSchemaMetaVersionKey,
                std::to_string(kStoreUserSchemaVersion));
  }
}

Store Store::open(const std::filesystem::path& cache_root) {
  Paths p;
  p.cache_root = cache_root;
  p.data_root = cache_root;
  return open(p);
}

Store Store::open(const Paths& paths) {
  namespace fs = std::filesystem;
  std::error_code ec;
  fs::create_directories(paths.cache_root, ec);
  if (ec) {
    throw std::runtime_error("create cache_root: " + ec.message());
  }
  fs::create_directories(paths.data_root, ec);
  if (ec) {
    throw std::runtime_error("create data_root: " + ec.message());
  }

  const fs::path index_path = paths.cache_root / "index.sqlite";
  const fs::path bulk_path = paths.cache_root / "bulk.sqlite";
  const fs::path user_path = paths.data_root / "user.sqlite";

  // Legacy thumtoo index (schema 1–4, content_id TEXT): replace index+bulk only.
  if (fs::exists(index_path)) {
    sqlite3* probe = nullptr;
    if (sqlite3_open(index_path.string().c_str(), &probe) == SQLITE_OK) {
      auto ver = meta_get_db(probe, kSchemaMetaVersionKey);
      sqlite3_close(probe);
      if (ver) {
        const int v = std::stoi(*ver);
        // Redesign epoch starts at 100. Pre-100 (ladder 1–4, etc.) is replaced.
        // 100 → current is handled by migrate_or_init_index (additive), not wipe.
        if (v < 100) {
          fs::remove(index_path, ec);
          fs::remove(bulk_path, ec);
          // WAL sidecars
          fs::remove(fs::path(index_path.string() + "-wal"), ec);
          fs::remove(fs::path(index_path.string() + "-shm"), ec);
          fs::remove(fs::path(bulk_path.string() + "-wal"), ec);
          fs::remove(fs::path(bulk_path.string() + "-shm"), ec);
        }
      }
    } else if (probe) {
      sqlite3_close(probe);
    }
  }

  sqlite3* index = open_sqlite(index_path);
  sqlite3* bulk = open_sqlite(bulk_path);
  sqlite3* user = open_sqlite(user_path);

  Store store(index, bulk, user, paths.cache_root, paths.data_root, 0);
  try {
    store.migrate_or_init_index();
    store.ensure_optional_index_tables();
    store.migrate_or_init_bulk();
    store.migrate_or_init_user();
  } catch (...) {
    // Store destructor will close.
    throw;
  }
  return store;
}

std::optional<std::string> Store::meta_get(std::string_view key) const {
  return meta_get_db(index_, key);
}

void Store::meta_set(std::string_view key, std::string_view value) {
  meta_set_db(index_, key, value);
}

std::int64_t Store::insert_blob(std::optional<std::int64_t> size,
                                BlobStatus status) {
  const std::int64_t now = now_unix_s();
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "INSERT INTO blob(size, status, created_at, updated_at) "
                         "VALUES(?1, ?2, ?3, ?4);",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare insert_blob");
  }
  if (size) {
    sqlite3_bind_int64(stmt, 1, *size);
  } else {
    sqlite3_bind_null(stmt, 1);
  }
  sqlite3_bind_int(stmt, 2, static_cast<int>(status));
  sqlite3_bind_int64(stmt, 3, now);
  sqlite3_bind_int64(stmt, 4, now);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step insert_blob");
  }
  sqlite3_finalize(stmt);
  return static_cast<std::int64_t>(sqlite3_last_insert_rowid(index_));
}

std::optional<Store::BlobRow> Store::find_blob(std::int64_t id) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT id, size, status, created_at, updated_at "
                         "FROM blob WHERE id = ?1;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare find_blob");
  }
  sqlite3_bind_int64(stmt, 1, id);
  std::optional<BlobRow> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    BlobRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
      r.size = sqlite3_column_int64(stmt, 1);
    }
    r.status = static_cast<BlobStatus>(sqlite3_column_int(stmt, 2));
    r.created_at = sqlite3_column_int64(stmt, 3);
    r.updated_at = sqlite3_column_int64(stmt, 4);
    out = std::move(r);
  }
  sqlite3_finalize(stmt);
  return out;
}

void Store::set_blob_size(std::int64_t id, std::int64_t size) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "UPDATE blob SET size = ?1, updated_at = ?2 WHERE id = ?3;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare set_blob_size");
  }
  sqlite3_bind_int64(stmt, 1, size);
  sqlite3_bind_int64(stmt, 2, now_unix_s());
  sqlite3_bind_int64(stmt, 3, id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step set_blob_size");
  }
  sqlite3_finalize(stmt);
}

void Store::set_blob_status(std::int64_t id, BlobStatus status) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "UPDATE blob SET status = ?1, updated_at = ?2 WHERE id = ?3;", -1,
          &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare set_blob_status");
  }
  sqlite3_bind_int(stmt, 1, static_cast<int>(status));
  sqlite3_bind_int64(stmt, 2, now_unix_s());
  sqlite3_bind_int64(stmt, 3, id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step set_blob_status");
  }
  sqlite3_finalize(stmt);
}



void Store::put_page_text_layer(std::int64_t blob_id, int page_1based,
                                std::string_view layout_key,
                                std::span<const std::uint8_t> data) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "INSERT INTO page_text_layer(blob_id, page_1based, layout_key, data) "
          "VALUES(?1, ?2, ?3, ?4) "
          "ON CONFLICT(blob_id, page_1based, layout_key) DO UPDATE SET "
          "data = excluded.data;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare put_page_text_layer");
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  sqlite3_bind_int(stmt, 2, page_1based);
  sqlite3_bind_text(stmt, 3, layout_key.data(), static_cast<int>(layout_key.size()),
                    SQLITE_STATIC);
  sqlite3_bind_blob(stmt, 4, data.data(), static_cast<int>(data.size()),
                    SQLITE_STATIC);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step put_page_text_layer");
  }
  sqlite3_finalize(stmt);
}

std::optional<std::vector<std::uint8_t>> Store::get_page_text_layer(
    std::int64_t blob_id, int page_1based, std::string_view layout_key) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "SELECT data FROM page_text_layer WHERE blob_id = ?1 AND "
          "page_1based = ?2 AND layout_key = ?3;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare get_page_text_layer");
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  sqlite3_bind_int(stmt, 2, page_1based);
  sqlite3_bind_text(stmt, 3, layout_key.data(), static_cast<int>(layout_key.size()),
                    SQLITE_STATIC);
  std::optional<std::vector<std::uint8_t>> out;
  if (sqlite3_step(stmt) == SQLITE_ROW &&
      sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
    const auto* p = static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt, 0));
    const int n = sqlite3_column_bytes(stmt, 0);
    if (p && n > 0) out = std::vector<std::uint8_t>(p, p + n);
  }
  sqlite3_finalize(stmt);
  return out;
}

std::int64_t Store::delete_page_text_layers(std::int64_t blob_id, int page_1based) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "DELETE FROM page_text_layer WHERE blob_id = ?1 AND page_1based = ?2;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare delete_page_text_layers");
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  sqlite3_bind_int(stmt, 2, page_1based);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step delete_page_text_layers");
  }
  const std::int64_t n = sqlite3_changes(index_);
  sqlite3_finalize(stmt);
  return n;
}

std::int64_t Store::delete_page_text_layers_for_blob(std::int64_t blob_id) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "DELETE FROM page_text_layer WHERE blob_id = ?1;", -1,
                         &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare delete_page_text_layers_for_blob");
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step delete_page_text_layers_for_blob");
  }
  const std::int64_t n = sqlite3_changes(index_);
  sqlite3_finalize(stmt);
  return n;
}

void Store::put_document_outline(std::int64_t blob_id,
                                 std::string_view layout_key,
                                 std::span<const std::uint8_t> data) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "INSERT INTO document_outline(blob_id, layout_key, data) "
          "VALUES(?1, ?2, ?3) "
          "ON CONFLICT(blob_id, layout_key) DO UPDATE SET data = excluded.data;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare put_document_outline");
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  sqlite3_bind_text(stmt, 2, layout_key.data(), static_cast<int>(layout_key.size()),
                    SQLITE_STATIC);
  sqlite3_bind_blob(stmt, 3, data.data(), static_cast<int>(data.size()),
                    SQLITE_STATIC);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step put_document_outline");
  }
  sqlite3_finalize(stmt);
}

std::optional<std::vector<std::uint8_t>> Store::get_document_outline(
    std::int64_t blob_id, std::string_view layout_key) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "SELECT data FROM document_outline WHERE blob_id = ?1 AND "
          "layout_key = ?2;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare get_document_outline");
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  sqlite3_bind_text(stmt, 2, layout_key.data(), static_cast<int>(layout_key.size()),
                    SQLITE_STATIC);
  std::optional<std::vector<std::uint8_t>> out;
  if (sqlite3_step(stmt) == SQLITE_ROW &&
      sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
    const auto* p = static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt, 0));
    const int n = sqlite3_column_bytes(stmt, 0);
    if (p && n > 0) out = std::vector<std::uint8_t>(p, p + n);
  }
  sqlite3_finalize(stmt);
  return out;
}

std::vector<Store::LocatorRow> Store::list_locators_like(
    std::string_view uri_like_pattern, int limit) const {
  if (uri_like_pattern.empty()) return {};
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT id, uri, blob_id, size, mtime_ns, updated_at, outer_path, "
                         "member_path FROM locator WHERE uri LIKE ?1 ESCAPE '\\' "
                         "ORDER BY uri LIMIT ?2;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare list_locators_like");
  }
  sqlite3_bind_text(stmt, 1, uri_like_pattern.data(),
                    static_cast<int>(uri_like_pattern.size()), SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, limit < 0 ? 0 : limit);
  std::vector<LocatorRow> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    LocatorRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.uri = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
      r.blob_id = sqlite3_column_int64(stmt, 2);
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL)
      r.size = sqlite3_column_int64(stmt, 3);
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL)
      r.mtime_ns = sqlite3_column_int64(stmt, 4);
    r.updated_at = sqlite3_column_int64(stmt, 5);
    if (sqlite3_column_count(stmt) > 6 &&
        sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
      r.outer_path =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    }
    if (sqlite3_column_count(stmt) > 7 &&
        sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
      r.member_path =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    }
    out.push_back(std::move(r));
  }
  sqlite3_finalize(stmt);
  return out;
}

void Store::put_blob_lqip(std::int64_t blob_id, int kind,
                          std::span<const std::uint8_t> data, int page_1based) {
  if (page_1based < 0) page_1based = 0;
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "INSERT INTO blob_lqip(blob_id, page_1based, kind, data) "
          "VALUES(?1, ?2, ?3, ?4) "
          "ON CONFLICT(blob_id, page_1based) DO UPDATE SET "
          "kind = excluded.kind, data = excluded.data;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    // Fallback: legacy single-column PK table without page_1based.
    if (sqlite3_prepare_v2(
            index_,
            "INSERT INTO blob_lqip(blob_id, kind, data) VALUES(?1, ?2, ?3) "
            "ON CONFLICT(blob_id) DO UPDATE SET kind = excluded.kind, "
            "data = excluded.data;",
            -1, &stmt, nullptr) != SQLITE_OK) {
      throw_sqlite(index_, "prepare put_blob_lqip");
    }
    sqlite3_bind_int64(stmt, 1, blob_id);
    sqlite3_bind_int(stmt, 2, kind);
    sqlite3_bind_blob(stmt, 3, data.data(), static_cast<int>(data.size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_int64(stmt, 1, blob_id);
    sqlite3_bind_int(stmt, 2, page_1based);
    sqlite3_bind_int(stmt, 3, kind);
    sqlite3_bind_blob(stmt, 4, data.data(), static_cast<int>(data.size()),
                      SQLITE_STATIC);
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step put_blob_lqip");
  }
  sqlite3_finalize(stmt);
}

std::optional<std::vector<std::uint8_t>> Store::get_blob_lqip(
    std::int64_t blob_id, int page_1based) const {
  if (page_1based < 0) page_1based = 0;
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "SELECT data FROM blob_lqip WHERE blob_id = ?1 AND page_1based = ?2;",
          -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int64(stmt, 1, blob_id);
    sqlite3_bind_int(stmt, 2, page_1based);
    std::optional<std::vector<std::uint8_t>> out;
    if (sqlite3_step(stmt) == SQLITE_ROW &&
        sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
      const auto* p =
          static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt, 0));
      const int n = sqlite3_column_bytes(stmt, 0);
      if (p && n > 0) out = std::vector<std::uint8_t>(p, p + n);
    }
    sqlite3_finalize(stmt);
    if (out) return out;
  } else if (stmt) {
    sqlite3_finalize(stmt);
  }
  // Legacy table (no page_1based column): only for page 0.
  if (page_1based != 0) return std::nullopt;
  if (sqlite3_prepare_v2(index_, "SELECT data FROM blob_lqip WHERE blob_id = ?1;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    return std::nullopt;
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  std::optional<std::vector<std::uint8_t>> out;
  if (sqlite3_step(stmt) == SQLITE_ROW &&
      sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
    const auto* p = static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt, 0));
    const int n = sqlite3_column_bytes(stmt, 0);
    if (p && n > 0) out = std::vector<std::uint8_t>(p, p + n);
  }
  sqlite3_finalize(stmt);
  return out;
}

std::optional<int> Store::get_blob_lqip_kind(std::int64_t blob_id,
                                            int page_1based) const {
  if (page_1based < 0) page_1based = 0;
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "SELECT kind FROM blob_lqip WHERE blob_id = ?1 AND page_1based = ?2;",
          -1, &stmt, nullptr) == SQLITE_OK) {
    sqlite3_bind_int64(stmt, 1, blob_id);
    sqlite3_bind_int(stmt, 2, page_1based);
    std::optional<int> out;
    if (sqlite3_step(stmt) == SQLITE_ROW) out = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    if (out) return out;
  } else if (stmt) {
    sqlite3_finalize(stmt);
  }
  if (page_1based != 0) return std::nullopt;
  if (sqlite3_prepare_v2(index_, "SELECT kind FROM blob_lqip WHERE blob_id = ?1;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    return std::nullopt;
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  std::optional<int> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) out = sqlite3_column_int(stmt, 0);
  sqlite3_finalize(stmt);
  return out;
}


void Store::put_hash(std::int64_t blob_id, HashAlgoId algo,
                     std::span<const std::uint8_t> digest) {
  if (digest.empty()) {
    throw std::invalid_argument("put_hash: empty digest");
  }
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "INSERT INTO blob_hash(blob_id, algo_id, digest) "
                         "VALUES(?1, ?2, ?3) "
                         "ON CONFLICT(blob_id, algo_id) DO UPDATE SET "
                         "digest = excluded.digest;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare put_hash");
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  sqlite3_bind_int(stmt, 2, static_cast<int>(algo));
  sqlite3_bind_blob(stmt, 3, digest.data(), static_cast<int>(digest.size()),
                    SQLITE_STATIC);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step put_hash");
  }
  sqlite3_finalize(stmt);
}

std::optional<std::int64_t> Store::find_blob_by_hash(
    HashAlgoId algo, std::span<const std::uint8_t> digest) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT blob_id FROM blob_hash "
                         "WHERE algo_id = ?1 AND digest = ?2;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare find_blob_by_hash");
  }
  sqlite3_bind_int(stmt, 1, static_cast<int>(algo));
  sqlite3_bind_blob(stmt, 2, digest.data(), static_cast<int>(digest.size()),
                    SQLITE_STATIC);
  std::optional<std::int64_t> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    out = sqlite3_column_int64(stmt, 0);
  }
  sqlite3_finalize(stmt);
  return out;
}

std::optional<std::vector<std::uint8_t>> Store::get_hash(
    std::int64_t blob_id, HashAlgoId algo) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT digest FROM blob_hash "
                         "WHERE blob_id = ?1 AND algo_id = ?2;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare get_hash");
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  sqlite3_bind_int(stmt, 2, static_cast<int>(algo));
  std::optional<std::vector<std::uint8_t>> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const void* p = sqlite3_column_blob(stmt, 0);
    const int n = sqlite3_column_bytes(stmt, 0);
    if (p && n > 0) {
      const auto* bytes = static_cast<const std::uint8_t*>(p);
      out = std::vector<std::uint8_t>(bytes, bytes + n);
    }
  }
  sqlite3_finalize(stmt);
  return out;
}

std::optional<std::string> Store::blob_ref_sha256(std::int64_t blob_id) const {
  auto d = get_hash(blob_id, HashAlgoId::Sha256);
  if (!d || d->size() != 32) return std::nullopt;
  return format_blob_ref_sha256(*d);
}

std::optional<std::vector<std::uint8_t>> Store::parse_sha256_digest(
    std::string_view ref_or_hex) {
  std::string_view hex = ref_or_hex;
  constexpr std::string_view kPref = "blob:sha256:";
  constexpr std::string_view kPrefLegacy = "sha256:";
  if (hex.size() >= kPref.size() &&
      hex.substr(0, kPref.size()) == kPref) {
    hex = hex.substr(kPref.size());
  } else if (hex.size() >= kPrefLegacy.size() &&
             hex.substr(0, kPrefLegacy.size()) == kPrefLegacy) {
    hex = hex.substr(kPrefLegacy.size());
  }
  if (hex.size() != 64) return std::nullopt;
  std::vector<std::uint8_t> out(32);
  for (std::size_t i = 0; i < 32; ++i) {
    const int hi = hex_nibble(hex[i * 2]);
    const int lo = hex_nibble(hex[i * 2 + 1]);
    if (hi < 0 || lo < 0) return std::nullopt;
    out[i] = static_cast<std::uint8_t>((hi << 4) | lo);
  }
  return out;
}

std::string Store::format_blob_ref_sha256(
    std::span<const std::uint8_t> digest32) {
  static constexpr char kHex[] = "0123456789abcdef";
  std::string out = "blob:sha256:";
  out.reserve(out.size() + digest32.size() * 2);
  for (std::uint8_t b : digest32) {
    out.push_back(kHex[(b >> 4) & 0xf]);
    out.push_back(kHex[b & 0xf]);
  }
  return out;
}

std::int64_t Store::upsert_locator(std::string_view uri,
                                   std::optional<std::int64_t> blob_id,
                                   std::optional<std::int64_t> size,
                                   std::optional<std::int64_t> mtime_ns,
                                   std::optional<std::string> outer_path,
                                   std::optional<std::string> member_path) {
  const std::int64_t now = now_unix_s();
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "INSERT INTO locator(uri, blob_id, size, mtime_ns, outer_path, "
          "member_path, updated_at) VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7) "
          "ON CONFLICT(uri) DO UPDATE SET "
          "blob_id = COALESCE(excluded.blob_id, locator.blob_id), "
          "size = COALESCE(excluded.size, locator.size), "
          "mtime_ns = COALESCE(excluded.mtime_ns, locator.mtime_ns), "
          "outer_path = COALESCE(excluded.outer_path, locator.outer_path), "
          "member_path = COALESCE(excluded.member_path, locator.member_path), "
          "updated_at = excluded.updated_at;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare upsert_locator");
  }
  sqlite3_bind_text(stmt, 1, uri.data(), static_cast<int>(uri.size()),
                    SQLITE_STATIC);
  if (blob_id) {
    sqlite3_bind_int64(stmt, 2, *blob_id);
  } else {
    sqlite3_bind_null(stmt, 2);
  }
  if (size) {
    sqlite3_bind_int64(stmt, 3, *size);
  } else {
    sqlite3_bind_null(stmt, 3);
  }
  if (mtime_ns) {
    sqlite3_bind_int64(stmt, 4, *mtime_ns);
  } else {
    sqlite3_bind_null(stmt, 4);
  }
  if (outer_path && !outer_path->empty()) {
    sqlite3_bind_text(stmt, 5, outer_path->c_str(), -1, SQLITE_TRANSIENT);
  } else {
    sqlite3_bind_null(stmt, 5);
  }
  if (member_path && !member_path->empty()) {
    sqlite3_bind_text(stmt, 6, member_path->c_str(), -1, SQLITE_TRANSIENT);
  } else {
    sqlite3_bind_null(stmt, 6);
  }
  sqlite3_bind_int64(stmt, 7, now);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step upsert_locator");
  }
  sqlite3_finalize(stmt);

  auto row = find_locator(uri);
  if (!row) {
    throw std::runtime_error("upsert_locator: row missing after write");
  }
  return row->id;
}

void Store::bind_locator_blob(std::string_view uri, std::int64_t blob_id) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "UPDATE locator SET blob_id = ?1, updated_at = ?2 "
                         "WHERE uri = ?3;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare bind_locator_blob");
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  sqlite3_bind_int64(stmt, 2, now_unix_s());
  sqlite3_bind_text(stmt, 3, uri.data(), static_cast<int>(uri.size()),
                    SQLITE_STATIC);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step bind_locator_blob");
  }
  sqlite3_finalize(stmt);
}

std::optional<Store::LocatorRow> Store::find_locator(
    std::string_view uri) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT id, uri, blob_id, size, mtime_ns, updated_at, outer_path, "
                         "member_path FROM locator WHERE uri = ?1;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare find_locator");
  }
  sqlite3_bind_text(stmt, 1, uri.data(), static_cast<int>(uri.size()),
                    SQLITE_STATIC);
  std::optional<LocatorRow> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    LocatorRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.uri = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
      r.blob_id = sqlite3_column_int64(stmt, 2);
    }
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
      r.size = sqlite3_column_int64(stmt, 3);
    }
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
      r.mtime_ns = sqlite3_column_int64(stmt, 4);
    }
    r.updated_at = sqlite3_column_int64(stmt, 5);
    if (sqlite3_column_count(stmt) > 6 &&
        sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
      r.outer_path =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    }
    if (sqlite3_column_count(stmt) > 7 &&
        sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
      r.member_path =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    }
    out = std::move(r);
  }
  sqlite3_finalize(stmt);
  return out;
}



std::vector<Store::LocatorRow> Store::list_locators_by_outer_path_prefix(
    std::string_view path_prefix, int limit) const {
  if (path_prefix.empty()) return {};
  std::string pattern;
  pattern.reserve(path_prefix.size() * 2 + 1);
  for (char ch : path_prefix) {
    if (ch == '%' || ch == '_' || ch == '\\') pattern.push_back('\\');
    pattern.push_back(ch);
  }
  pattern.push_back('%');
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "SELECT id, uri, blob_id, size, mtime_ns, updated_at, outer_path, "
          "member_path FROM locator WHERE outer_path LIKE ?1 ESCAPE '\\' "
          "ORDER BY outer_path LIMIT ?2;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare list_locators_by_outer_path_prefix");
  }
  sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, limit < 0 ? 0 : limit);
  std::vector<LocatorRow> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    LocatorRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.uri = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
      r.blob_id = sqlite3_column_int64(stmt, 2);
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL)
      r.size = sqlite3_column_int64(stmt, 3);
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL)
      r.mtime_ns = sqlite3_column_int64(stmt, 4);
    r.updated_at = sqlite3_column_int64(stmt, 5);
    if (sqlite3_column_type(stmt, 6) != SQLITE_NULL)
      r.outer_path =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    if (sqlite3_column_type(stmt, 7) != SQLITE_NULL)
      r.member_path =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    out.push_back(std::move(r));
  }
  sqlite3_finalize(stmt);
  return out;
}

std::vector<Store::LocatorRow> Store::list_locators(int limit) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT id, uri, blob_id, size, mtime_ns, updated_at, outer_path, "
                         "member_path FROM locator ORDER BY id LIMIT ?1;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare list_locators");
  }
  sqlite3_bind_int(stmt, 1, limit < 0 ? 0 : limit);
  std::vector<LocatorRow> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    LocatorRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.uri = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
      r.blob_id = sqlite3_column_int64(stmt, 2);
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL)
      r.size = sqlite3_column_int64(stmt, 3);
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL)
      r.mtime_ns = sqlite3_column_int64(stmt, 4);
    r.updated_at = sqlite3_column_int64(stmt, 5);
    if (sqlite3_column_count(stmt) > 6 &&
        sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
      r.outer_path =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    }
    if (sqlite3_column_count(stmt) > 7 &&
        sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
      r.member_path =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    }
    out.push_back(std::move(r));
  }
  sqlite3_finalize(stmt);
  return out;
}

std::vector<Store::LocatorRow> Store::list_locators_by_uri_prefix(
    std::string_view uri_prefix, int limit) const {
  if (uri_prefix.empty()) return {};
  std::string pattern;
  pattern.reserve(uri_prefix.size() * 2 + 1);
  for (char ch : uri_prefix) {
    if (ch == '%' || ch == '_' || ch == '\\') pattern.push_back('\\');
    pattern.push_back(ch);
  }
  pattern.push_back('%');
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT id, uri, blob_id, size, mtime_ns, updated_at, outer_path, "
                         "member_path FROM locator WHERE uri LIKE ?1 ESCAPE '\\' "
                         "ORDER BY uri LIMIT ?2;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare list_locators_by_uri_prefix");
  }
  sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
  sqlite3_bind_int(stmt, 2, limit < 0 ? 0 : limit);
  std::vector<LocatorRow> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    LocatorRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.uri = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL)
      r.blob_id = sqlite3_column_int64(stmt, 2);
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL)
      r.size = sqlite3_column_int64(stmt, 3);
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL)
      r.mtime_ns = sqlite3_column_int64(stmt, 4);
    r.updated_at = sqlite3_column_int64(stmt, 5);
    if (sqlite3_column_count(stmt) > 6 &&
        sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
      r.outer_path =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    }
    if (sqlite3_column_count(stmt) > 7 &&
        sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
      r.member_path =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    }
    out.push_back(std::move(r));
  }
  sqlite3_finalize(stmt);
  return out;
}

std::vector<Store::LocatorRow> Store::list_locators_for_blob(
    std::int64_t blob_id, int limit) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT id, uri, blob_id, size, mtime_ns, updated_at, outer_path, "
                         "member_path FROM locator WHERE blob_id = ?1 LIMIT ?2;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare list_locators_for_blob");
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  sqlite3_bind_int(stmt, 2, limit);
  std::vector<LocatorRow> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    LocatorRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.uri = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
      r.blob_id = sqlite3_column_int64(stmt, 2);
    }
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
      r.size = sqlite3_column_int64(stmt, 3);
    }
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
      r.mtime_ns = sqlite3_column_int64(stmt, 4);
    }
    r.updated_at = sqlite3_column_int64(stmt, 5);
    if (sqlite3_column_count(stmt) > 6 &&
        sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
      r.outer_path =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
    }
    if (sqlite3_column_count(stmt) > 7 &&
        sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
      r.member_path =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
    }
    out.push_back(std::move(r));
  }
  sqlite3_finalize(stmt);
  return out;
}


std::optional<std::int64_t> Store::delete_locator(std::string_view uri) {
  auto existing = find_locator(uri);
  if (!existing) return std::nullopt;
  const auto blob_id = existing->blob_id;
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_, "DELETE FROM locator WHERE uri = ?1;", -1,
                         &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare delete_locator");
  }
  sqlite3_bind_text(stmt, 1, uri.data(), static_cast<int>(uri.size()),
                    SQLITE_STATIC);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step delete_locator");
  }
  sqlite3_finalize(stmt);
  return blob_id;
}

std::int64_t Store::delete_locators_with_uri_prefix(std::string_view uri_prefix) {
  if (uri_prefix.empty()) return 0;
  sqlite3_stmt* stmt = nullptr;
  // Literal prefix: uri >= prefix AND uri < prefix with last char bumped is
  // fragile; use LIKE with escaped %/_ in prefix + '%'.
  std::string pattern;
  pattern.reserve(uri_prefix.size() * 2 + 1);
  for (char c : uri_prefix) {
    if (c == '%' || c == '_' || c == '\\') pattern.push_back('\\');
    pattern.push_back(c);
  }
  pattern.push_back('%');
  if (sqlite3_prepare_v2(index_,
                         "DELETE FROM locator WHERE uri LIKE ?1 ESCAPE '\\';",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare delete_locators_with_uri_prefix");
  }
  sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step delete_locators_with_uri_prefix");
  }
  const auto n = static_cast<std::int64_t>(sqlite3_changes(index_));
  sqlite3_finalize(stmt);
  return n;
}

std::int64_t Store::purge_blob_if_unreferenced(std::int64_t blob_id) {
  // Still referenced?
  auto locs = list_locators_for_blob(blob_id, 1);
  if (!locs.empty()) return 0;

  // Collect media ids for bulk tile cleanup before CASCADE delete.
  std::vector<std::int64_t> media_ids;
  {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(index_, "SELECT id FROM media WHERE blob_id = ?1;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
      throw_sqlite(index_, "prepare purge media list");
    }
    sqlite3_bind_int64(stmt, 1, blob_id);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      media_ids.push_back(sqlite3_column_int64(stmt, 0));
    }
    sqlite3_finalize(stmt);
  }

  std::int64_t tiles = 0;
  for (const auto mid : media_ids) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(bulk_, "DELETE FROM tile_blob WHERE media_id = ?1;",
                           -1, &stmt, nullptr) != SQLITE_OK) {
      throw_sqlite(bulk_, "prepare purge tile_blob");
    }
    sqlite3_bind_int64(stmt, 1, mid);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
      sqlite3_finalize(stmt);
      throw_sqlite(bulk_, "step purge tile_blob");
    }
    tiles += sqlite3_changes(bulk_);
    sqlite3_finalize(stmt);
  }

  // blob_hash and media/region/tile index cascade via FK when blob deleted.
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_, "DELETE FROM blob WHERE id = ?1;", -1, &stmt,
                         nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare purge blob");
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step purge blob");
  }
  sqlite3_finalize(stmt);
  return tiles;
}

Store::ForgetStats Store::forget_uri(std::string_view uri, bool dry_run) {
  ForgetStats st;
  auto loc = find_locator(uri);
  if (!loc) return st;
  st.locator_removed = true;
  const auto blob_id = loc->blob_id;
  if (dry_run) {
    if (blob_id) {
      auto remaining = list_locators_for_blob(*blob_id, 2);
      // Would purge if this is the only locator.
      if (remaining.size() <= 1) st.blob_purged = true;
    }
    return st;
  }
  (void)delete_locator(uri);
  if (blob_id) {
    const auto tiles = purge_blob_if_unreferenced(*blob_id);
    if (tiles > 0 || !find_blob(*blob_id)) {
      // find_blob gone => purged
      st.blob_purged = !find_blob(*blob_id).has_value();
      st.tiles_deleted = tiles;
    }
  }
  return st;
}



Store::PrefixForgetStats Store::forget_uri_prefix(std::string_view uri_prefix,
                                                  bool dry_run) {
  PrefixForgetStats st;
  if (uri_prefix.empty()) return st;

  std::string pattern;
  pattern.reserve(uri_prefix.size() * 2 + 1);
  for (char ch : uri_prefix) {
    if (ch == '%' || ch == '_' || ch == '\\') pattern.push_back('\\');
    pattern.push_back(ch);
  }
  pattern.push_back('%');

  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT uri FROM locator WHERE uri LIKE ?1 ESCAPE '\\';",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare forget_uri_prefix list");
  }
  sqlite3_bind_text(stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);
  std::vector<std::string> uris;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    const char* u = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (u) uris.emplace_back(u);
  }
  sqlite3_finalize(stmt);

  for (const auto& u : uris) {
    auto one = forget_uri(u, dry_run);
    if (one.locator_removed) ++st.locators_removed;
    if (one.blob_purged) ++st.blobs_purged;
    st.tiles_deleted += one.tiles_deleted;
  }
  return st;
}

std::vector<std::int64_t> Store::list_orphan_blob_ids(int limit) const {
  sqlite3_stmt* stmt = nullptr;
  // blobs with no locator pointing at them
  if (sqlite3_prepare_v2(
          index_,
          "SELECT b.id FROM blob b WHERE NOT EXISTS ("
          "  SELECT 1 FROM locator l WHERE l.blob_id = b.id"
          ") LIMIT ?1;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare list_orphan_blob_ids");
  }
  sqlite3_bind_int(stmt, 1, limit < 0 ? 0 : limit);
  std::vector<std::int64_t> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    out.push_back(sqlite3_column_int64(stmt, 0));
  }
  sqlite3_finalize(stmt);
  return out;
}

Store::OrphanPurgeStats Store::purge_orphan_blobs(bool dry_run, int limit) {
  OrphanPurgeStats st;
  auto ids = list_orphan_blob_ids(limit);
  st.blobs_purged = static_cast<std::int64_t>(ids.size());
  if (dry_run) return st;
  for (const auto id : ids) {
    st.tiles_deleted += purge_blob_if_unreferenced(id);
  }
  // Recompute blobs_purged as how many actually vanished
  std::int64_t gone = 0;
  for (const auto id : ids) {
    if (!find_blob(id)) ++gone;
  }
  st.blobs_purged = gone;
  return st;
}

std::int64_t Store::count_blobs() const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_, "SELECT COUNT(*) FROM blob;", -1, &stmt,
                         nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare count_blobs");
  }
  std::int64_t n = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int64(stmt, 0);
  sqlite3_finalize(stmt);
  return n;
}

std::int64_t Store::count_locators() const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_, "SELECT COUNT(*) FROM locator;", -1, &stmt,
                         nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare count_locators");
  }
  std::int64_t n = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int64(stmt, 0);
  sqlite3_finalize(stmt);
  return n;
}

std::int64_t Store::insert_media(std::int64_t blob_id, MediaKind kind,
                                 std::optional<int> width,
                                 std::optional<int> height,
                                 MediaStatus status) {
  const std::int64_t now = now_unix_s();
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "INSERT INTO media(blob_id, kind, width, height, "
                         "status, updated_at) VALUES(?1, ?2, ?3, ?4, ?5, ?6);",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare insert_media");
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  sqlite3_bind_int(stmt, 2, static_cast<int>(kind));
  if (width) {
    sqlite3_bind_int(stmt, 3, *width);
  } else {
    sqlite3_bind_null(stmt, 3);
  }
  if (height) {
    sqlite3_bind_int(stmt, 4, *height);
  } else {
    sqlite3_bind_null(stmt, 4);
  }
  sqlite3_bind_int(stmt, 5, static_cast<int>(status));
  sqlite3_bind_int64(stmt, 6, now);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step insert_media");
  }
  sqlite3_finalize(stmt);
  return static_cast<std::int64_t>(sqlite3_last_insert_rowid(index_));
}

std::int64_t Store::ensure_image_media(std::int64_t blob_id,
                                       std::optional<int> width,
                                       std::optional<int> height) {
  if (auto existing = find_media_for_blob(blob_id, MediaKind::Image)) {
    if (width && height) {
      set_media_size(existing->id, *width, *height);
    }
    (void)ensure_region(existing->id, RegionKind::Full, "", std::nullopt);
    return existing->id;
  }
  const std::int64_t media_id =
      insert_media(blob_id, MediaKind::Image, width, height, MediaStatus::Ready);
  (void)ensure_region(media_id, RegionKind::Full, "", std::nullopt);
  return media_id;
}

std::optional<Store::MediaRow> Store::find_media(std::int64_t media_id) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT id, blob_id, kind, width, height, duration_ms, "
                         "page_count, still_count, status, error_code, "
                         "updated_at FROM media WHERE id = ?1;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare find_media");
  }
  sqlite3_bind_int64(stmt, 1, media_id);
  std::optional<MediaRow> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    MediaRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.blob_id = sqlite3_column_int64(stmt, 1);
    r.kind = static_cast<MediaKind>(sqlite3_column_int(stmt, 2));
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
      r.width = sqlite3_column_int(stmt, 3);
    }
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
      r.height = sqlite3_column_int(stmt, 4);
    }
    if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
      r.duration_ms = sqlite3_column_int64(stmt, 5);
    }
    if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
      r.page_count = sqlite3_column_int(stmt, 6);
    }
    if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
      r.still_count = sqlite3_column_int(stmt, 7);
    }
    r.status = static_cast<MediaStatus>(sqlite3_column_int(stmt, 8));
    if (sqlite3_column_type(stmt, 9) != SQLITE_NULL) {
      r.error_code =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
    }
    r.updated_at = sqlite3_column_int64(stmt, 10);
    out = std::move(r);
  }
  sqlite3_finalize(stmt);
  return out;
}

std::optional<Store::MediaRow> Store::find_media_for_blob(
    std::int64_t blob_id, MediaKind kind) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT id FROM media WHERE blob_id = ?1 AND kind = ?2;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare find_media_for_blob");
  }
  sqlite3_bind_int64(stmt, 1, blob_id);
  sqlite3_bind_int(stmt, 2, static_cast<int>(kind));
  std::optional<std::int64_t> id;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    id = sqlite3_column_int64(stmt, 0);
  }
  sqlite3_finalize(stmt);
  if (!id) return std::nullopt;
  return find_media(*id);
}

void Store::set_media_size(std::int64_t media_id, int width, int height) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "UPDATE media SET width = ?1, height = ?2, "
                         "updated_at = ?3 WHERE id = ?4;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare set_media_size");
  }
  sqlite3_bind_int(stmt, 1, width);
  sqlite3_bind_int(stmt, 2, height);
  sqlite3_bind_int64(stmt, 3, now_unix_s());
  sqlite3_bind_int64(stmt, 4, media_id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step set_media_size");
  }
  sqlite3_finalize(stmt);
}

void Store::set_region_size(std::int64_t region_id, int width, int height) {
  if (width < 1 || height < 1) {
    throw std::invalid_argument("set_region_size: width and height must be >= 1");
  }
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "UPDATE region SET width = ?1, height = ?2 "
                         "WHERE id = ?3;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare set_region_size");
  }
  sqlite3_bind_int(stmt, 1, width);
  sqlite3_bind_int(stmt, 2, height);
  sqlite3_bind_int64(stmt, 3, region_id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step set_region_size");
  }
  sqlite3_finalize(stmt);
}


void Store::set_media_page_count(std::int64_t media_id, int page_count) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "UPDATE media SET page_count = ?1, updated_at = ?2 "
                         "WHERE id = ?3;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare set_media_page_count");
  }
  sqlite3_bind_int(stmt, 1, page_count);
  sqlite3_bind_int64(stmt, 2, now_unix_s());
  sqlite3_bind_int64(stmt, 3, media_id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step set_media_page_count");
  }
  sqlite3_finalize(stmt);
}

void Store::set_media_status(std::int64_t media_id, MediaStatus status,
                             std::optional<std::string_view> error_code) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "UPDATE media SET status = ?1, error_code = ?2, "
                         "updated_at = ?3 WHERE id = ?4;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare set_media_status");
  }
  sqlite3_bind_int(stmt, 1, static_cast<int>(status));
  if (error_code) {
    sqlite3_bind_text(stmt, 2, error_code->data(),
                      static_cast<int>(error_code->size()), SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 2);
  }
  sqlite3_bind_int64(stmt, 3, now_unix_s());
  sqlite3_bind_int64(stmt, 4, media_id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step set_media_status");
  }
  sqlite3_finalize(stmt);
}

std::int64_t Store::insert_region(std::int64_t media_id, RegionKind kind,
                                  std::string_view key,
                                  std::optional<int> ordinal) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "INSERT INTO region(media_id, kind, key, ordinal) "
                         "VALUES(?1, ?2, ?3, ?4);",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare insert_region");
  }
  sqlite3_bind_int64(stmt, 1, media_id);
  sqlite3_bind_int(stmt, 2, static_cast<int>(kind));
  sqlite3_bind_text(stmt, 3, key.data(), static_cast<int>(key.size()),
                    SQLITE_STATIC);
  if (ordinal) {
    sqlite3_bind_int(stmt, 4, *ordinal);
  } else {
    sqlite3_bind_null(stmt, 4);
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step insert_region");
  }
  sqlite3_finalize(stmt);
  return static_cast<std::int64_t>(sqlite3_last_insert_rowid(index_));
}

std::int64_t Store::ensure_region(std::int64_t media_id, RegionKind kind,
                                  std::string_view key,
                                  std::optional<int> ordinal) {
  if (auto existing = find_region_by_key(media_id, kind, key)) {
    return existing->id;
  }
  return insert_region(media_id, kind, key, ordinal);
}

std::optional<Store::RegionRow> Store::find_region(
    std::int64_t region_id) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT id, media_id, kind, key, ordinal, width, height FROM region "
                         "WHERE id = ?1;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare find_region");
  }
  sqlite3_bind_int64(stmt, 1, region_id);
  std::optional<RegionRow> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    RegionRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.media_id = sqlite3_column_int64(stmt, 1);
    r.kind = static_cast<RegionKind>(sqlite3_column_int(stmt, 2));
    r.key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
      r.ordinal = sqlite3_column_int(stmt, 4);
    }
    // width/height are selected above — must load them or get_size always misses
    // and hosts re-probe every page (open PDF) despite a settled Store.
    if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
      r.width = sqlite3_column_int(stmt, 5);
    }
    if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
      r.height = sqlite3_column_int(stmt, 6);
    }
    out = std::move(r);
  }
  sqlite3_finalize(stmt);
  return out;
}

std::optional<Store::RegionRow> Store::find_region_by_key(
    std::int64_t media_id, RegionKind kind, std::string_view key) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT id, media_id, kind, key, ordinal, width, height "
                         "FROM region WHERE media_id = ?1 AND kind = ?2 AND "
                         "key = ?3;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare find_region_by_key");
  }
  sqlite3_bind_int64(stmt, 1, media_id);
  sqlite3_bind_int(stmt, 2, static_cast<int>(kind));
  sqlite3_bind_text(stmt, 3, key.data(), static_cast<int>(key.size()),
                    SQLITE_STATIC);
  std::optional<RegionRow> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    RegionRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.media_id = sqlite3_column_int64(stmt, 1);
    r.kind = static_cast<RegionKind>(sqlite3_column_int(stmt, 2));
    r.key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
      r.ordinal = sqlite3_column_int(stmt, 4);
    }
    if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
      r.width = sqlite3_column_int(stmt, 5);
    }
    if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
      r.height = sqlite3_column_int(stmt, 6);
    }
    out = std::move(r);
  }
  sqlite3_finalize(stmt);
  return out;
}

std::optional<Store::RegionRow> Store::find_full_region(
    std::int64_t media_id) const {
  return find_region_by_key(media_id, RegionKind::Full, "");
}

void Store::put_tile(const TileRow& meta, std::span<const std::uint8_t> data) {
  if (data.empty()) {
    throw std::invalid_argument("put_tile: empty data");
  }
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "INSERT INTO tile(media_id, region_id, scale, x, y, "
                         "width, height, codec_id, quality) "
                         "VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9) "
                         "ON CONFLICT(media_id, region_id, scale, x, y) DO UPDATE SET "
                         "width = excluded.width, height = excluded.height, "
                         "codec_id = excluded.codec_id, quality = excluded.quality;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare put_tile index");
  }
  sqlite3_bind_int64(stmt, 1, meta.media_id);
  sqlite3_bind_int64(stmt, 2, meta.region_id);
  sqlite3_bind_int(stmt, 3, meta.scale);
  sqlite3_bind_int(stmt, 4, meta.x);
  sqlite3_bind_int(stmt, 5, meta.y);
  sqlite3_bind_int(stmt, 6, meta.width);
  sqlite3_bind_int(stmt, 7, meta.height);
  sqlite3_bind_int(stmt, 8, static_cast<int>(meta.codec_id));
  if (meta.quality) {
    sqlite3_bind_int(stmt, 9, *meta.quality);
  } else {
    sqlite3_bind_null(stmt, 9);
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step put_tile index");
  }
  sqlite3_finalize(stmt);

  if (sqlite3_prepare_v2(bulk_,
                         "INSERT INTO tile_blob(media_id, region_id, scale, x, "
                         "y, codec_id, data) VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7) "
                         "ON CONFLICT(media_id, region_id, scale, x, y) DO UPDATE SET "
                         "codec_id = excluded.codec_id, data = excluded.data;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(bulk_, "prepare put_tile bulk");
  }
  sqlite3_bind_int64(stmt, 1, meta.media_id);
  sqlite3_bind_int64(stmt, 2, meta.region_id);
  sqlite3_bind_int(stmt, 3, meta.scale);
  sqlite3_bind_int(stmt, 4, meta.x);
  sqlite3_bind_int(stmt, 5, meta.y);
  sqlite3_bind_int(stmt, 6, static_cast<int>(meta.codec_id));
  sqlite3_bind_blob(stmt, 7, data.data(), static_cast<int>(data.size()),
                    SQLITE_STATIC);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(bulk_, "step put_tile bulk");
  }
  sqlite3_finalize(stmt);
}

bool Store::has_tile(std::int64_t media_id, std::int64_t region_id, int scale,
                     int x, int y) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT 1 FROM tile WHERE media_id = ?1 AND "
                         "region_id = ?2 AND scale = ?3 AND x = ?4 AND y = ?5;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare has_tile");
  }
  sqlite3_bind_int64(stmt, 1, media_id);
  sqlite3_bind_int64(stmt, 2, region_id);
  sqlite3_bind_int(stmt, 3, scale);
  sqlite3_bind_int(stmt, 4, x);
  sqlite3_bind_int(stmt, 5, y);
  const bool ok = sqlite3_step(stmt) == SQLITE_ROW;
  sqlite3_finalize(stmt);
  return ok;
}

std::optional<Store::TileRow> Store::find_tile_meta(std::int64_t media_id,
                                                    std::int64_t region_id,
                                                    int scale, int x,
                                                    int y) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT media_id, region_id, scale, x, y, width, "
                         "height, codec_id, quality FROM tile WHERE "
                         "media_id = ?1 AND region_id = ?2 AND scale = ?3 AND "
                         "x = ?4 AND y = ?5;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare find_tile_meta");
  }
  sqlite3_bind_int64(stmt, 1, media_id);
  sqlite3_bind_int64(stmt, 2, region_id);
  sqlite3_bind_int(stmt, 3, scale);
  sqlite3_bind_int(stmt, 4, x);
  sqlite3_bind_int(stmt, 5, y);
  std::optional<TileRow> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    TileRow r;
    r.media_id = sqlite3_column_int64(stmt, 0);
    r.region_id = sqlite3_column_int64(stmt, 1);
    r.scale = sqlite3_column_int(stmt, 2);
    r.x = sqlite3_column_int(stmt, 3);
    r.y = sqlite3_column_int(stmt, 4);
    r.width = sqlite3_column_int(stmt, 5);
    r.height = sqlite3_column_int(stmt, 6);
    r.codec_id = static_cast<CodecId>(sqlite3_column_int(stmt, 7));
    if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
      r.quality = sqlite3_column_int(stmt, 8);
    }
    out = r;
  }
  sqlite3_finalize(stmt);
  return out;
}

std::optional<std::vector<std::uint8_t>> Store::get_tile_data(
    std::int64_t media_id, std::int64_t region_id, int scale, int x,
    int y) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(bulk_,
                         "SELECT data FROM tile_blob WHERE media_id = ?1 AND "
                         "region_id = ?2 AND scale = ?3 AND x = ?4 AND y = ?5;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(bulk_, "prepare get_tile_data");
  }
  sqlite3_bind_int64(stmt, 1, media_id);
  sqlite3_bind_int64(stmt, 2, region_id);
  sqlite3_bind_int(stmt, 3, scale);
  sqlite3_bind_int(stmt, 4, x);
  sqlite3_bind_int(stmt, 5, y);
  std::optional<std::vector<std::uint8_t>> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    const void* p = sqlite3_column_blob(stmt, 0);
    const int n = sqlite3_column_bytes(stmt, 0);
    if (p && n > 0) {
      const auto* bytes = static_cast<const std::uint8_t*>(p);
      out = std::vector<std::uint8_t>(bytes, bytes + n);
    }
  }
  sqlite3_finalize(stmt);
  return out;
}

void Store::delete_tiles_for_region(std::int64_t media_id,
                                    std::int64_t region_id) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "DELETE FROM tile WHERE media_id = ?1 AND "
                         "region_id = ?2;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare delete_tiles index");
  }
  sqlite3_bind_int64(stmt, 1, media_id);
  sqlite3_bind_int64(stmt, 2, region_id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step delete_tiles index");
  }
  sqlite3_finalize(stmt);

  if (sqlite3_prepare_v2(bulk_,
                         "DELETE FROM tile_blob WHERE media_id = ?1 AND "
                         "region_id = ?2;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(bulk_, "prepare delete_tiles bulk");
  }
  sqlite3_bind_int64(stmt, 1, media_id);
  sqlite3_bind_int64(stmt, 2, region_id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(bulk_, "step delete_tiles bulk");
  }
  sqlite3_finalize(stmt);
}

std::int64_t Store::count_tiles() const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_, "SELECT COUNT(*) FROM tile;", -1, &stmt,
                         nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare count_tiles");
  }
  std::int64_t n = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int64(stmt, 0);
  sqlite3_finalize(stmt);
  return n;
}

std::int64_t Store::count_media() const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_, "SELECT COUNT(*) FROM media;", -1, &stmt,
                         nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare count_media");
  }
  std::int64_t n = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int64(stmt, 0);
  sqlite3_finalize(stmt);
  return n;
}

std::int64_t Store::count_regions() const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_, "SELECT COUNT(*) FROM region;", -1, &stmt,
                         nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare count_regions");
  }
  std::int64_t n = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int64(stmt, 0);
  sqlite3_finalize(stmt);
  return n;
}

void Store::upsert_container_member(std::int64_t container_id,
                                    std::string_view member_path,
                                    bool is_directory,
                                    std::optional<std::int64_t> uncompressed_size,
                                    std::optional<std::int64_t> blob_id) {
  sqlite3_stmt* stmt = nullptr;
  // Preserve existing blob_id when caller passes nullopt (TOC refresh without hash).
  if (sqlite3_prepare_v2(
          index_,
          "INSERT INTO container_member(container_id, member_path, is_directory, "
          "uncompressed_size, blob_id) VALUES(?1, ?2, ?3, ?4, ?5) "
          "ON CONFLICT(container_id, member_path) DO UPDATE SET "
          "is_directory = excluded.is_directory, "
          "uncompressed_size = COALESCE(excluded.uncompressed_size, "
          "container_member.uncompressed_size), "
          "blob_id = COALESCE(excluded.blob_id, container_member.blob_id);",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare upsert_container_member");
  }
  sqlite3_bind_int64(stmt, 1, container_id);
  sqlite3_bind_text(stmt, 2, member_path.data(),
                    static_cast<int>(member_path.size()), SQLITE_STATIC);
  sqlite3_bind_int(stmt, 3, is_directory ? 1 : 0);
  if (uncompressed_size) {
    sqlite3_bind_int64(stmt, 4, *uncompressed_size);
  } else {
    sqlite3_bind_null(stmt, 4);
  }
  if (blob_id) {
    sqlite3_bind_int64(stmt, 5, *blob_id);
  } else {
    sqlite3_bind_null(stmt, 5);
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step upsert_container_member");
  }
  sqlite3_finalize(stmt);
}

void Store::replace_container_members(
    std::int64_t container_id, const std::vector<ContainerMemberRow>& members) {
  exec_index("BEGIN IMMEDIATE;");
  try {
    sqlite3_stmt* del = nullptr;
    if (sqlite3_prepare_v2(index_,
                           "DELETE FROM container_member WHERE container_id = ?1;",
                           -1, &del, nullptr) != SQLITE_OK) {
      throw_sqlite(index_, "prepare replace_container_members delete");
    }
    sqlite3_bind_int64(del, 1, container_id);
    if (sqlite3_step(del) != SQLITE_DONE) {
      sqlite3_finalize(del);
      throw_sqlite(index_, "step replace_container_members delete");
    }
    sqlite3_finalize(del);
    for (const auto& m : members) {
      upsert_container_member(container_id, m.member_path, m.is_directory,
                              m.uncompressed_size, m.blob_id);
    }
    exec_index("COMMIT;");
  } catch (...) {
    exec_index("ROLLBACK;");
    throw;
  }
}

std::vector<Store::ContainerMemberRow> Store::list_container_members(
    std::int64_t container_id, int limit) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT container_id, member_path, is_directory, "
                         "uncompressed_size, blob_id FROM container_member "
                         "WHERE container_id = ?1 ORDER BY member_path LIMIT ?2;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare list_container_members");
  }
  sqlite3_bind_int64(stmt, 1, container_id);
  sqlite3_bind_int(stmt, 2, limit);
  std::vector<ContainerMemberRow> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    ContainerMemberRow r;
    r.container_id = sqlite3_column_int64(stmt, 0);
    r.member_path =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    r.is_directory = sqlite3_column_int(stmt, 2) != 0;
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
      r.uncompressed_size = sqlite3_column_int64(stmt, 3);
    }
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
      r.blob_id = sqlite3_column_int64(stmt, 4);
    }
    out.push_back(std::move(r));
  }
  sqlite3_finalize(stmt);
  return out;
}

std::optional<Store::ContainerMemberRow> Store::find_container_member(
    std::int64_t container_id, std::string_view member_path) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT container_id, member_path, is_directory, "
                         "uncompressed_size, blob_id FROM container_member "
                         "WHERE container_id = ?1 AND member_path = ?2;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare find_container_member");
  }
  sqlite3_bind_int64(stmt, 1, container_id);
  sqlite3_bind_text(stmt, 2, member_path.data(),
                    static_cast<int>(member_path.size()), SQLITE_STATIC);
  std::optional<ContainerMemberRow> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    ContainerMemberRow r;
    r.container_id = sqlite3_column_int64(stmt, 0);
    r.member_path =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    r.is_directory = sqlite3_column_int(stmt, 2) != 0;
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
      r.uncompressed_size = sqlite3_column_int64(stmt, 3);
    }
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
      r.blob_id = sqlite3_column_int64(stmt, 4);
    }
    out = std::move(r);
  }
  sqlite3_finalize(stmt);
  return out;
}

void Store::set_container_member_blob(std::int64_t container_id,
                                      std::string_view member_path,
                                      std::int64_t member_blob_id) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "UPDATE container_member SET blob_id = ?1 "
                         "WHERE container_id = ?2 AND member_path = ?3;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare set_container_member_blob");
  }
  sqlite3_bind_int64(stmt, 1, member_blob_id);
  sqlite3_bind_int64(stmt, 2, container_id);
  sqlite3_bind_text(stmt, 3, member_path.data(),
                    static_cast<int>(member_path.size()), SQLITE_STATIC);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step set_container_member_blob");
  }
  sqlite3_finalize(stmt);
}

std::int64_t Store::count_container_members(std::int64_t container_id) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_,
                         "SELECT COUNT(*) FROM container_member "
                         "WHERE container_id = ?1;",
                         -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare count_container_members");
  }
  sqlite3_bind_int64(stmt, 1, container_id);
  std::int64_t n = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int64(stmt, 0);
  sqlite3_finalize(stmt);
  return n;
}

std::int64_t Store::ensure_document_media(std::int64_t blob_id,
                                          std::optional<int> page_count) {
  if (auto existing = find_media_for_blob(blob_id, MediaKind::Document)) {
    if (page_count) set_media_page_count(existing->id, *page_count);
    return existing->id;
  }
  const std::int64_t media_id = insert_media(
      blob_id, MediaKind::Document, {}, {}, MediaStatus::Ready);
  if (page_count) set_media_page_count(media_id, *page_count);
  return media_id;
}

std::int64_t Store::ensure_page_region(std::int64_t media_id, int page_1based) {
  return ensure_page_region(media_id, page_1based, std::to_string(page_1based));
}

std::int64_t Store::ensure_page_region(std::int64_t media_id, int page_1based,
                                       std::string_view region_key) {
  if (page_1based < 1) {
    throw std::invalid_argument("ensure_page_region: page must be >= 1");
  }
  if (region_key.empty()) {
    return ensure_region(media_id, RegionKind::Page, std::to_string(page_1based),
                         page_1based);
  }
  return ensure_region(media_id, RegionKind::Page, region_key, page_1based);
}

void Store::replace_directory_snapshot(
    const DirectorySnapshotRow& snap,
    const std::vector<DirectoryEntryRow>& entries) {
  if (snap.dir_uri.empty()) {
    throw std::invalid_argument("replace_directory_snapshot: empty dir_uri");
  }
  const std::int64_t listed_at =
      snap.listed_at != 0 ? snap.listed_at : now_unix_s();
  auto bind_opt_i64 = [](sqlite3_stmt* st, int idx,
                         const std::optional<std::int64_t>& v) {
    if (v) {
      sqlite3_bind_int64(st, idx, *v);
    } else {
      sqlite3_bind_null(st, idx);
    }
  };
  auto bind_opt_text = [](sqlite3_stmt* st, int idx,
                          const std::optional<std::string>& v) {
    if (v) {
      sqlite3_bind_text(st, idx, v->data(), static_cast<int>(v->size()),
                        SQLITE_STATIC);
    } else {
      sqlite3_bind_null(st, idx);
    }
  };
  exec_index("BEGIN IMMEDIATE;");
  try {
    sqlite3_stmt* del = nullptr;
    if (sqlite3_prepare_v2(
            index_, "DELETE FROM directory_snapshot WHERE dir_uri = ?1;", -1,
            &del, nullptr) != SQLITE_OK) {
      throw_sqlite(index_, "prepare replace_directory_snapshot delete");
    }
    sqlite3_bind_text(del, 1, snap.dir_uri.data(),
                      static_cast<int>(snap.dir_uri.size()), SQLITE_STATIC);
    if (sqlite3_step(del) != SQLITE_DONE) {
      sqlite3_finalize(del);
      throw_sqlite(index_, "step replace_directory_snapshot delete");
    }
    sqlite3_finalize(del);

    sqlite3_stmt* ins = nullptr;
    if (sqlite3_prepare_v2(
            index_,
            "INSERT INTO directory_snapshot(dir_uri, size, mtime_ns, listed_at, "
            "incomplete, inode, dev, nlink, mode, uid, gid, error_code) "
            "VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12);",
            -1, &ins, nullptr) != SQLITE_OK) {
      throw_sqlite(index_, "prepare replace_directory_snapshot insert");
    }
    sqlite3_bind_text(ins, 1, snap.dir_uri.data(),
                      static_cast<int>(snap.dir_uri.size()), SQLITE_STATIC);
    bind_opt_i64(ins, 2, snap.size);
    bind_opt_i64(ins, 3, snap.mtime_ns);
    sqlite3_bind_int64(ins, 4, listed_at);
    sqlite3_bind_int(ins, 5, snap.incomplete ? 1 : 0);
    bind_opt_i64(ins, 6, snap.inode);
    bind_opt_i64(ins, 7, snap.dev);
    bind_opt_i64(ins, 8, snap.nlink);
    bind_opt_i64(ins, 9, snap.mode);
    bind_opt_i64(ins, 10, snap.uid);
    bind_opt_i64(ins, 11, snap.gid);
    bind_opt_text(ins, 12, snap.error_code);
    if (sqlite3_step(ins) != SQLITE_DONE) {
      sqlite3_finalize(ins);
      throw_sqlite(index_, "step replace_directory_snapshot insert");
    }
    sqlite3_finalize(ins);

    sqlite3_stmt* ent = nullptr;
    if (sqlite3_prepare_v2(
            index_,
            "INSERT INTO directory_entry(dir_uri, name, child_uri, is_dir, size, "
            "mtime_ns, file_type, mode, uid, gid, atime_ns, ctime_ns, birth_ns, "
            "nlink, inode, dev, rdev, symlink_target, blob_id, flags) "
            "VALUES(?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14,?15,?16,?17,"
            "?18,?19,?20);",
            -1, &ent, nullptr) != SQLITE_OK) {
      throw_sqlite(index_, "prepare replace_directory_snapshot entry");
    }
    for (const auto& e : entries) {
      if (e.name.empty()) continue;
      sqlite3_reset(ent);
      sqlite3_clear_bindings(ent);
      sqlite3_bind_text(ent, 1, snap.dir_uri.data(),
                        static_cast<int>(snap.dir_uri.size()), SQLITE_STATIC);
      sqlite3_bind_text(ent, 2, e.name.data(), static_cast<int>(e.name.size()),
                        SQLITE_STATIC);
      bind_opt_text(ent, 3, e.child_uri);
      const bool is_dir =
          e.is_dir || (e.file_type && *e.file_type == kFsFileTypeDir);
      sqlite3_bind_int(ent, 4, is_dir ? 1 : 0);
      bind_opt_i64(ent, 5, e.size);
      bind_opt_i64(ent, 6, e.mtime_ns);
      if (e.file_type) {
        bind_opt_i64(ent, 7, e.file_type);
      } else if (e.is_dir) {
        sqlite3_bind_int64(ent, 7, kFsFileTypeDir);
      } else {
        sqlite3_bind_null(ent, 7);
      }
      bind_opt_i64(ent, 8, e.mode);
      bind_opt_i64(ent, 9, e.uid);
      bind_opt_i64(ent, 10, e.gid);
      bind_opt_i64(ent, 11, e.atime_ns);
      bind_opt_i64(ent, 12, e.ctime_ns);
      bind_opt_i64(ent, 13, e.birth_ns);
      bind_opt_i64(ent, 14, e.nlink);
      bind_opt_i64(ent, 15, e.inode);
      bind_opt_i64(ent, 16, e.dev);
      bind_opt_i64(ent, 17, e.rdev);
      bind_opt_text(ent, 18, e.symlink_target);
      bind_opt_i64(ent, 19, e.blob_id);
      if (e.flags) {
        sqlite3_bind_int64(ent, 20, *e.flags);
      } else {
        sqlite3_bind_int64(ent, 20, 0);
      }
      if (sqlite3_step(ent) != SQLITE_DONE) {
        sqlite3_finalize(ent);
        throw_sqlite(index_, "step replace_directory_snapshot entry");
      }
    }
    sqlite3_finalize(ent);
    exec_index("COMMIT;");
  } catch (...) {
    exec_index("ROLLBACK;");
    throw;
  }
}

std::optional<Store::DirectorySnapshotRow> Store::find_directory_snapshot(
    std::string_view dir_uri) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "SELECT dir_uri, size, mtime_ns, listed_at, incomplete, "
          "inode, dev, nlink, mode, uid, gid, error_code "
          "FROM directory_snapshot WHERE dir_uri = ?1;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare find_directory_snapshot");
  }
  sqlite3_bind_text(stmt, 1, dir_uri.data(), static_cast<int>(dir_uri.size()),
                    SQLITE_STATIC);
  std::optional<DirectorySnapshotRow> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    DirectorySnapshotRow r;
    r.dir_uri = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
      r.size = sqlite3_column_int64(stmt, 1);
    }
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
      r.mtime_ns = sqlite3_column_int64(stmt, 2);
    }
    r.listed_at = sqlite3_column_int64(stmt, 3);
    r.incomplete = sqlite3_column_int(stmt, 4) != 0;
    if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
      r.inode = sqlite3_column_int64(stmt, 5);
    }
    if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
      r.dev = sqlite3_column_int64(stmt, 6);
    }
    if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
      r.nlink = sqlite3_column_int64(stmt, 7);
    }
    if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
      r.mode = sqlite3_column_int64(stmt, 8);
    }
    if (sqlite3_column_type(stmt, 9) != SQLITE_NULL) {
      r.uid = sqlite3_column_int64(stmt, 9);
    }
    if (sqlite3_column_type(stmt, 10) != SQLITE_NULL) {
      r.gid = sqlite3_column_int64(stmt, 10);
    }
    if (sqlite3_column_type(stmt, 11) != SQLITE_NULL) {
      r.error_code =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
    }
    out = std::move(r);
  }
  sqlite3_finalize(stmt);
  return out;
}

std::vector<Store::DirectoryEntryRow> Store::list_directory_entries(
    std::string_view dir_uri, int limit) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "SELECT dir_uri, name, child_uri, is_dir, size, mtime_ns, "
          "file_type, mode, uid, gid, atime_ns, ctime_ns, birth_ns, "
          "nlink, inode, dev, rdev, symlink_target, blob_id, flags "
          "FROM directory_entry WHERE dir_uri = ?1 ORDER BY name LIMIT ?2;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare list_directory_entries");
  }
  sqlite3_bind_text(stmt, 1, dir_uri.data(), static_cast<int>(dir_uri.size()),
                    SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, limit);
  std::vector<DirectoryEntryRow> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    DirectoryEntryRow r;
    r.dir_uri = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    r.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
      r.child_uri =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    }
    r.is_dir = sqlite3_column_int(stmt, 3) != 0;
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
      r.size = sqlite3_column_int64(stmt, 4);
    }
    if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) {
      r.mtime_ns = sqlite3_column_int64(stmt, 5);
    }
    if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) {
      r.file_type = sqlite3_column_int64(stmt, 6);
    }
    if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) {
      r.mode = sqlite3_column_int64(stmt, 7);
    }
    if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
      r.uid = sqlite3_column_int64(stmt, 8);
    }
    if (sqlite3_column_type(stmt, 9) != SQLITE_NULL) {
      r.gid = sqlite3_column_int64(stmt, 9);
    }
    if (sqlite3_column_type(stmt, 10) != SQLITE_NULL) {
      r.atime_ns = sqlite3_column_int64(stmt, 10);
    }
    if (sqlite3_column_type(stmt, 11) != SQLITE_NULL) {
      r.ctime_ns = sqlite3_column_int64(stmt, 11);
    }
    if (sqlite3_column_type(stmt, 12) != SQLITE_NULL) {
      r.birth_ns = sqlite3_column_int64(stmt, 12);
    }
    if (sqlite3_column_type(stmt, 13) != SQLITE_NULL) {
      r.nlink = sqlite3_column_int64(stmt, 13);
    }
    if (sqlite3_column_type(stmt, 14) != SQLITE_NULL) {
      r.inode = sqlite3_column_int64(stmt, 14);
    }
    if (sqlite3_column_type(stmt, 15) != SQLITE_NULL) {
      r.dev = sqlite3_column_int64(stmt, 15);
    }
    if (sqlite3_column_type(stmt, 16) != SQLITE_NULL) {
      r.rdev = sqlite3_column_int64(stmt, 16);
    }
    if (sqlite3_column_type(stmt, 17) != SQLITE_NULL) {
      r.symlink_target =
          reinterpret_cast<const char*>(sqlite3_column_text(stmt, 17));
    }
    if (sqlite3_column_type(stmt, 18) != SQLITE_NULL) {
      r.blob_id = sqlite3_column_int64(stmt, 18);
    }
    if (sqlite3_column_type(stmt, 19) != SQLITE_NULL) {
      r.flags = sqlite3_column_int64(stmt, 19);
    }
    out.push_back(std::move(r));
  }
  sqlite3_finalize(stmt);
  return out;
}

void Store::delete_directory_snapshot(std::string_view dir_uri) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_, "DELETE FROM directory_snapshot WHERE dir_uri = ?1;", -1,
          &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare delete_directory_snapshot");
  }
  sqlite3_bind_text(stmt, 1, dir_uri.data(), static_cast<int>(dir_uri.size()),
                    SQLITE_STATIC);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(index_, "step delete_directory_snapshot");
  }
  sqlite3_finalize(stmt);
}

std::int64_t Store::count_directory_snapshots() const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(index_, "SELECT COUNT(*) FROM directory_snapshot;", -1,
                         &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare count_directory_snapshots");
  }
  std::int64_t n = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int64(stmt, 0);
  sqlite3_finalize(stmt);
  return n;
}

std::int64_t Store::ensure_tag_def(std::string_view name,
                                   std::optional<std::string_view> label,
                                   std::optional<std::string_view> color,
                                   std::optional<std::string_view> badge) {
  if (name.empty()) {
    throw std::invalid_argument("ensure_tag_def: empty name");
  }
  if (auto existing = find_tag_def_by_name(name)) {
    return existing->id;
  }
  const std::int64_t now = now_unix_s();
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "INSERT INTO tag_def(name, label, color, badge, created_at) "
          "VALUES(?1,?2,?3,?4,?5);",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare ensure_tag_def");
  }
  sqlite3_bind_text(stmt, 1, name.data(), static_cast<int>(name.size()),
                    SQLITE_STATIC);
  if (label) {
    sqlite3_bind_text(stmt, 2, label->data(), static_cast<int>(label->size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 2);
  }
  if (color) {
    sqlite3_bind_text(stmt, 3, color->data(), static_cast<int>(color->size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 3);
  }
  if (badge) {
    sqlite3_bind_text(stmt, 4, badge->data(), static_cast<int>(badge->size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 4);
  }
  sqlite3_bind_int64(stmt, 5, now);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    // Race: another writer may have inserted the same name.
    if (auto existing = find_tag_def_by_name(name)) {
      return existing->id;
    }
    throw_sqlite(user_, "step ensure_tag_def");
  }
  const std::int64_t id = sqlite3_last_insert_rowid(user_);
  sqlite3_finalize(stmt);
  return id;
}

std::optional<Store::TagDefRow> Store::find_tag_def(std::int64_t id) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "SELECT id, name, label, color, badge, created_at FROM tag_def "
          "WHERE id = ?1;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare find_tag_def");
  }
  sqlite3_bind_int64(stmt, 1, id);
  std::optional<TagDefRow> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    TagDefRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
      r.label = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    }
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
      r.color = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    }
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
      r.badge = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    }
    r.created_at = sqlite3_column_int64(stmt, 5);
    out = std::move(r);
  }
  sqlite3_finalize(stmt);
  return out;
}

std::optional<Store::TagDefRow> Store::find_tag_def_by_name(
    std::string_view name) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "SELECT id, name, label, color, badge, created_at FROM tag_def "
          "WHERE name = ?1;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare find_tag_def_by_name");
  }
  sqlite3_bind_text(stmt, 1, name.data(), static_cast<int>(name.size()),
                    SQLITE_STATIC);
  std::optional<TagDefRow> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    TagDefRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
      r.label = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    }
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
      r.color = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    }
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
      r.badge = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    }
    r.created_at = sqlite3_column_int64(stmt, 5);
    out = std::move(r);
  }
  sqlite3_finalize(stmt);
  return out;
}

void Store::add_blob_tag(std::string_view blob_ref, std::string_view tag_name,
                         std::string_view source) {
  if (blob_ref.empty() || tag_name.empty()) return;
  const std::int64_t tag_id = ensure_tag_def(tag_name);
  const std::int64_t now = now_unix_s();
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "INSERT OR IGNORE INTO blob_tag(blob_ref, tag_id, tagged_at, source) "
          "VALUES(?1,?2,?3,?4);",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare add_blob_tag");
  }
  sqlite3_bind_text(stmt, 1, blob_ref.data(), static_cast<int>(blob_ref.size()),
                    SQLITE_STATIC);
  sqlite3_bind_int64(stmt, 2, tag_id);
  sqlite3_bind_int64(stmt, 3, now);
  if (source.empty()) {
    sqlite3_bind_null(stmt, 4);
  } else {
    sqlite3_bind_text(stmt, 4, source.data(), static_cast<int>(source.size()),
                      SQLITE_STATIC);
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step add_blob_tag");
  }
  sqlite3_finalize(stmt);
}

bool Store::remove_blob_tag(std::string_view blob_ref,
                            std::string_view tag_name) {
  if (blob_ref.empty() || tag_name.empty()) return false;
  auto def = find_tag_def_by_name(tag_name);
  if (!def) return false;
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "DELETE FROM blob_tag WHERE blob_ref = ?1 AND tag_id = ?2;", -1, &stmt,
          nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare remove_blob_tag");
  }
  sqlite3_bind_text(stmt, 1, blob_ref.data(), static_cast<int>(blob_ref.size()),
                    SQLITE_STATIC);
  sqlite3_bind_int64(stmt, 2, def->id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step remove_blob_tag");
  }
  const int changes = sqlite3_changes(user_);
  sqlite3_finalize(stmt);
  return changes > 0;
}

std::vector<std::string> Store::tags_for_blob_ref(
    std::string_view blob_ref) const {
  std::vector<std::string> out;
  if (blob_ref.empty()) return out;
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "SELECT d.name FROM blob_tag t JOIN tag_def d ON d.id = t.tag_id "
          "WHERE t.blob_ref = ?1 ORDER BY d.name;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare tags_for_blob_ref");
  }
  sqlite3_bind_text(stmt, 1, blob_ref.data(), static_cast<int>(blob_ref.size()),
                    SQLITE_STATIC);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    out.emplace_back(
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
  }
  sqlite3_finalize(stmt);
  return out;
}

std::vector<std::string> Store::blob_refs_for_tag(std::string_view tag_name,
                                                  int limit) const {
  std::vector<std::string> out;
  if (tag_name.empty()) return out;
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "SELECT t.blob_ref FROM blob_tag t JOIN tag_def d ON d.id = t.tag_id "
          "WHERE d.name = ?1 ORDER BY t.blob_ref LIMIT ?2;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare blob_refs_for_tag");
  }
  sqlite3_bind_text(stmt, 1, tag_name.data(), static_cast<int>(tag_name.size()),
                    SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, limit);
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    out.emplace_back(
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
  }
  sqlite3_finalize(stmt);
  return out;
}

std::int64_t Store::create_collection(std::optional<std::string_view> label,
                                      std::optional<std::string_view> color) {
  const std::int64_t now = now_unix_s();
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "INSERT INTO collection(label, color, created_at, updated_at) "
          "VALUES(?1,?2,?3,?4);",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare create_collection");
  }
  if (label) {
    sqlite3_bind_text(stmt, 1, label->data(), static_cast<int>(label->size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 1);
  }
  if (color) {
    sqlite3_bind_text(stmt, 2, color->data(), static_cast<int>(color->size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 2);
  }
  sqlite3_bind_int64(stmt, 3, now);
  sqlite3_bind_int64(stmt, 4, now);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step create_collection");
  }
  const std::int64_t id = sqlite3_last_insert_rowid(user_);
  sqlite3_finalize(stmt);
  return id;
}

std::optional<Store::CollectionRow> Store::find_collection(
    std::int64_t id) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "SELECT id, label, color, created_at, updated_at FROM collection "
          "WHERE id = ?1;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare find_collection");
  }
  sqlite3_bind_int64(stmt, 1, id);
  std::optional<CollectionRow> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    CollectionRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) {
      r.label = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    }
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
      r.color = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    }
    r.created_at = sqlite3_column_int64(stmt, 3);
    r.updated_at = sqlite3_column_int64(stmt, 4);
    out = std::move(r);
  }
  sqlite3_finalize(stmt);
  return out;
}

void Store::set_collection_label(std::int64_t id,
                                 std::optional<std::string_view> label) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "UPDATE collection SET label = ?1, updated_at = ?2 WHERE id = ?3;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare set_collection_label");
  }
  if (label) {
    sqlite3_bind_text(stmt, 1, label->data(), static_cast<int>(label->size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 1);
  }
  sqlite3_bind_int64(stmt, 2, now_unix_s());
  sqlite3_bind_int64(stmt, 3, id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step set_collection_label");
  }
  sqlite3_finalize(stmt);
}

void Store::set_collection_color(std::int64_t id,
                                 std::optional<std::string_view> color) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "UPDATE collection SET color = ?1, updated_at = ?2 WHERE id = ?3;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare set_collection_color");
  }
  if (color) {
    sqlite3_bind_text(stmt, 1, color->data(), static_cast<int>(color->size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 1);
  }
  sqlite3_bind_int64(stmt, 2, now_unix_s());
  sqlite3_bind_int64(stmt, 3, id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step set_collection_color");
  }
  sqlite3_finalize(stmt);
}

void Store::upsert_collection_member(std::int64_t collection_id,
                                     std::string_view blob_ref,
                                     std::optional<int> ordinal,
                                     std::optional<std::string_view> path_key) {
  if (blob_ref.empty()) {
    throw std::invalid_argument("upsert_collection_member: empty blob_ref");
  }
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "INSERT INTO collection_member(collection_id, blob_ref, ordinal, "
          "path_key) VALUES(?1,?2,?3,?4) "
          "ON CONFLICT(collection_id, blob_ref) DO UPDATE SET "
          "ordinal = excluded.ordinal, path_key = excluded.path_key;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare upsert_collection_member");
  }
  sqlite3_bind_int64(stmt, 1, collection_id);
  sqlite3_bind_text(stmt, 2, blob_ref.data(), static_cast<int>(blob_ref.size()),
                    SQLITE_STATIC);
  if (ordinal) {
    sqlite3_bind_int(stmt, 3, *ordinal);
  } else {
    sqlite3_bind_null(stmt, 3);
  }
  if (path_key) {
    sqlite3_bind_text(stmt, 4, path_key->data(),
                      static_cast<int>(path_key->size()), SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 4);
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step upsert_collection_member");
  }
  sqlite3_finalize(stmt);
}

bool Store::remove_collection_member(std::int64_t collection_id,
                                     std::string_view blob_ref) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "DELETE FROM collection_member WHERE collection_id = ?1 AND "
          "blob_ref = ?2;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare remove_collection_member");
  }
  sqlite3_bind_int64(stmt, 1, collection_id);
  sqlite3_bind_text(stmt, 2, blob_ref.data(), static_cast<int>(blob_ref.size()),
                    SQLITE_STATIC);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step remove_collection_member");
  }
  const int changes = sqlite3_changes(user_);
  sqlite3_finalize(stmt);
  return changes > 0;
}

std::vector<Store::CollectionMemberRow> Store::list_collection_members(
    std::int64_t collection_id, int limit) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "SELECT collection_id, blob_ref, ordinal, path_key "
          "FROM collection_member WHERE collection_id = ?1 "
          "ORDER BY ordinal IS NULL, ordinal, blob_ref LIMIT ?2;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare list_collection_members");
  }
  sqlite3_bind_int64(stmt, 1, collection_id);
  sqlite3_bind_int(stmt, 2, limit);
  std::vector<CollectionMemberRow> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    CollectionMemberRow r;
    r.collection_id = sqlite3_column_int64(stmt, 0);
    r.blob_ref = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
      r.ordinal = sqlite3_column_int(stmt, 2);
    }
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
      r.path_key = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    }
    out.push_back(std::move(r));
  }
  sqlite3_finalize(stmt);
  return out;
}

void Store::delete_collection(std::int64_t id) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(user_, "DELETE FROM collection WHERE id = ?1;", -1,
                         &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare delete_collection");
  }
  sqlite3_bind_int64(stmt, 1, id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step delete_collection");
  }
  sqlite3_finalize(stmt);
}

std::int64_t Store::count_collections() const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(user_, "SELECT COUNT(*) FROM collection;", -1, &stmt,
                         nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare count_collections");
  }
  std::int64_t n = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int64(stmt, 0);
  sqlite3_finalize(stmt);
  return n;
}

std::int64_t Store::create_bookmark(std::string_view target_ref,
                                    std::optional<std::string_view> title) {
  if (target_ref.empty()) {
    throw std::invalid_argument("create_bookmark: empty target_ref");
  }
  const std::int64_t now = now_unix_s();
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "INSERT INTO bookmark(target_ref, title, created_at, updated_at) "
          "VALUES(?1,?2,?3,?4);",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare create_bookmark");
  }
  sqlite3_bind_text(stmt, 1, target_ref.data(),
                    static_cast<int>(target_ref.size()), SQLITE_STATIC);
  if (title) {
    sqlite3_bind_text(stmt, 2, title->data(), static_cast<int>(title->size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 2);
  }
  sqlite3_bind_int64(stmt, 3, now);
  sqlite3_bind_int64(stmt, 4, now);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step create_bookmark");
  }
  const std::int64_t id = sqlite3_last_insert_rowid(user_);
  sqlite3_finalize(stmt);
  return id;
}

std::optional<Store::BookmarkRow> Store::find_bookmark(std::int64_t id) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "SELECT id, target_ref, title, created_at, updated_at FROM bookmark "
          "WHERE id = ?1;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare find_bookmark");
  }
  sqlite3_bind_int64(stmt, 1, id);
  std::optional<BookmarkRow> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    BookmarkRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.target_ref =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
      r.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    }
    r.created_at = sqlite3_column_int64(stmt, 3);
    r.updated_at = sqlite3_column_int64(stmt, 4);
    out = std::move(r);
  }
  sqlite3_finalize(stmt);
  return out;
}

void Store::set_bookmark_title(std::int64_t id,
                               std::optional<std::string_view> title) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "UPDATE bookmark SET title = ?1, updated_at = ?2 WHERE id = ?3;", -1,
          &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare set_bookmark_title");
  }
  if (title) {
    sqlite3_bind_text(stmt, 1, title->data(), static_cast<int>(title->size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 1);
  }
  sqlite3_bind_int64(stmt, 2, now_unix_s());
  sqlite3_bind_int64(stmt, 3, id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step set_bookmark_title");
  }
  sqlite3_finalize(stmt);
}

void Store::delete_bookmark(std::int64_t id) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(user_, "DELETE FROM bookmark WHERE id = ?1;", -1,
                         &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare delete_bookmark");
  }
  sqlite3_bind_int64(stmt, 1, id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step delete_bookmark");
  }
  sqlite3_finalize(stmt);
}

std::vector<Store::BookmarkRow> Store::list_bookmarks_for_target(
    std::string_view target_ref, int limit) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "SELECT id, target_ref, title, created_at, updated_at FROM bookmark "
          "WHERE target_ref = ?1 ORDER BY id LIMIT ?2;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare list_bookmarks_for_target");
  }
  sqlite3_bind_text(stmt, 1, target_ref.data(),
                    static_cast<int>(target_ref.size()), SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, limit);
  std::vector<BookmarkRow> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    BookmarkRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.target_ref =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
      r.title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    }
    r.created_at = sqlite3_column_int64(stmt, 3);
    r.updated_at = sqlite3_column_int64(stmt, 4);
    out.push_back(std::move(r));
  }
  sqlite3_finalize(stmt);
  return out;
}

std::int64_t Store::count_bookmarks() const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(user_, "SELECT COUNT(*) FROM bookmark;", -1, &stmt,
                         nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare count_bookmarks");
  }
  std::int64_t n = 0;
  if (sqlite3_step(stmt) == SQLITE_ROW) n = sqlite3_column_int64(stmt, 0);
  sqlite3_finalize(stmt);
  return n;
}

std::int64_t Store::add_link_edge(std::string_view from_ref,
                                  std::string_view to_ref,
                                  std::optional<std::string_view> rel,
                                  int source) {
  if (from_ref.empty() || to_ref.empty()) {
    throw std::invalid_argument("add_link_edge: empty from_ref or to_ref");
  }
  {
    sqlite3_stmt* find = nullptr;
    if (rel) {
      if (sqlite3_prepare_v2(
              user_,
              "SELECT id FROM link_edge WHERE from_ref = ?1 AND to_ref = ?2 "
              "AND rel = ?3 AND source = ?4;",
              -1, &find, nullptr) != SQLITE_OK) {
        throw_sqlite(user_, "prepare add_link_edge find");
      }
      sqlite3_bind_text(find, 1, from_ref.data(),
                        static_cast<int>(from_ref.size()), SQLITE_STATIC);
      sqlite3_bind_text(find, 2, to_ref.data(), static_cast<int>(to_ref.size()),
                        SQLITE_STATIC);
      sqlite3_bind_text(find, 3, rel->data(), static_cast<int>(rel->size()),
                        SQLITE_STATIC);
      sqlite3_bind_int(find, 4, source);
    } else {
      if (sqlite3_prepare_v2(
              user_,
              "SELECT id FROM link_edge WHERE from_ref = ?1 AND to_ref = ?2 "
              "AND rel IS NULL AND source = ?3;",
              -1, &find, nullptr) != SQLITE_OK) {
        throw_sqlite(user_, "prepare add_link_edge find null rel");
      }
      sqlite3_bind_text(find, 1, from_ref.data(),
                        static_cast<int>(from_ref.size()), SQLITE_STATIC);
      sqlite3_bind_text(find, 2, to_ref.data(), static_cast<int>(to_ref.size()),
                        SQLITE_STATIC);
      sqlite3_bind_int(find, 3, source);
    }
    if (sqlite3_step(find) == SQLITE_ROW) {
      const std::int64_t id = sqlite3_column_int64(find, 0);
      sqlite3_finalize(find);
      return id;
    }
    sqlite3_finalize(find);
  }

  const std::int64_t now = now_unix_s();
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "INSERT INTO link_edge(from_ref, to_ref, rel, source, created_at) "
          "VALUES(?1,?2,?3,?4,?5);",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare add_link_edge");
  }
  sqlite3_bind_text(stmt, 1, from_ref.data(), static_cast<int>(from_ref.size()),
                    SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, to_ref.data(), static_cast<int>(to_ref.size()),
                    SQLITE_STATIC);
  if (rel) {
    sqlite3_bind_text(stmt, 3, rel->data(), static_cast<int>(rel->size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 3);
  }
  sqlite3_bind_int(stmt, 4, source);
  sqlite3_bind_int64(stmt, 5, now);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step add_link_edge");
  }
  const std::int64_t id = sqlite3_last_insert_rowid(user_);
  sqlite3_finalize(stmt);
  return id;
}

bool Store::remove_link_edge(std::int64_t id) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(user_, "DELETE FROM link_edge WHERE id = ?1;", -1,
                         &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare remove_link_edge");
  }
  sqlite3_bind_int64(stmt, 1, id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step remove_link_edge");
  }
  const int changes = sqlite3_changes(user_);
  sqlite3_finalize(stmt);
  return changes > 0;
}

std::vector<Store::LinkEdgeRow> Store::list_links_from(std::string_view from_ref,
                                                       int limit) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "SELECT id, from_ref, to_ref, rel, source, created_at FROM link_edge "
          "WHERE from_ref = ?1 ORDER BY id LIMIT ?2;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare list_links_from");
  }
  sqlite3_bind_text(stmt, 1, from_ref.data(), static_cast<int>(from_ref.size()),
                    SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, limit);
  std::vector<LinkEdgeRow> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    LinkEdgeRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.from_ref = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    r.to_ref = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
      r.rel = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    }
    r.source = sqlite3_column_int(stmt, 4);
    r.created_at = sqlite3_column_int64(stmt, 5);
    out.push_back(std::move(r));
  }
  sqlite3_finalize(stmt);
  return out;
}

std::vector<Store::LinkEdgeRow> Store::list_links_to(std::string_view to_ref,
                                                     int limit) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "SELECT id, from_ref, to_ref, rel, source, created_at FROM link_edge "
          "WHERE to_ref = ?1 ORDER BY id LIMIT ?2;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare list_links_to");
  }
  sqlite3_bind_text(stmt, 1, to_ref.data(), static_cast<int>(to_ref.size()),
                    SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, limit);
  std::vector<LinkEdgeRow> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    LinkEdgeRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.from_ref = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    r.to_ref = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
      r.rel = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    }
    r.source = sqlite3_column_int(stmt, 4);
    r.created_at = sqlite3_column_int64(stmt, 5);
    out.push_back(std::move(r));
  }
  sqlite3_finalize(stmt);
  return out;
}

std::int64_t Store::create_annotation(std::string_view target_ref, int kind,
                                      std::optional<std::string_view> body,
                                      std::span<const std::uint8_t> geom) {
  if (target_ref.empty()) {
    throw std::invalid_argument("create_annotation: empty target_ref");
  }
  const std::int64_t now = now_unix_s();
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "INSERT INTO annotation(target_ref, kind, body, geom, created_at, "
          "updated_at) VALUES(?1,?2,?3,?4,?5,?6);",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare create_annotation");
  }
  sqlite3_bind_text(stmt, 1, target_ref.data(),
                    static_cast<int>(target_ref.size()), SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, kind);
  if (body) {
    sqlite3_bind_text(stmt, 3, body->data(), static_cast<int>(body->size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 3);
  }
  if (!geom.empty()) {
    sqlite3_bind_blob(stmt, 4, geom.data(), static_cast<int>(geom.size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 4);
  }
  sqlite3_bind_int64(stmt, 5, now);
  sqlite3_bind_int64(stmt, 6, now);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step create_annotation");
  }
  const std::int64_t id = sqlite3_last_insert_rowid(user_);
  sqlite3_finalize(stmt);
  return id;
}

std::optional<Store::AnnotationRow> Store::find_annotation(
    std::int64_t id) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "SELECT id, target_ref, kind, body, geom, created_at, updated_at "
          "FROM annotation WHERE id = ?1;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare find_annotation");
  }
  sqlite3_bind_int64(stmt, 1, id);
  std::optional<AnnotationRow> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    AnnotationRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.target_ref =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    r.kind = sqlite3_column_int(stmt, 2);
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
      r.body = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    }
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
      const auto* p =
          static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt, 4));
      const int n = sqlite3_column_bytes(stmt, 4);
      r.geom.assign(p, p + n);
    }
    r.created_at = sqlite3_column_int64(stmt, 5);
    r.updated_at = sqlite3_column_int64(stmt, 6);
    out = std::move(r);
  }
  sqlite3_finalize(stmt);
  return out;
}

void Store::set_annotation_body(std::int64_t id,
                                std::optional<std::string_view> body) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "UPDATE annotation SET body = ?1, updated_at = ?2 WHERE id = ?3;", -1,
          &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare set_annotation_body");
  }
  if (body) {
    sqlite3_bind_text(stmt, 1, body->data(), static_cast<int>(body->size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 1);
  }
  sqlite3_bind_int64(stmt, 2, now_unix_s());
  sqlite3_bind_int64(stmt, 3, id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step set_annotation_body");
  }
  sqlite3_finalize(stmt);
}

void Store::set_annotation_geom(std::int64_t id,
                                std::span<const std::uint8_t> geom) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "UPDATE annotation SET geom = ?1, updated_at = ?2 WHERE id = ?3;", -1,
          &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare set_annotation_geom");
  }
  if (!geom.empty()) {
    sqlite3_bind_blob(stmt, 1, geom.data(), static_cast<int>(geom.size()),
                      SQLITE_STATIC);
  } else {
    sqlite3_bind_null(stmt, 1);
  }
  sqlite3_bind_int64(stmt, 2, now_unix_s());
  sqlite3_bind_int64(stmt, 3, id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step set_annotation_geom");
  }
  sqlite3_finalize(stmt);
}

void Store::delete_annotation(std::int64_t id) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(user_, "DELETE FROM annotation WHERE id = ?1;", -1,
                         &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare delete_annotation");
  }
  sqlite3_bind_int64(stmt, 1, id);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(user_, "step delete_annotation");
  }
  sqlite3_finalize(stmt);
}

std::vector<Store::AnnotationRow> Store::list_annotations_for_target(
    std::string_view target_ref, int limit) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          user_,
          "SELECT id, target_ref, kind, body, geom, created_at, updated_at "
          "FROM annotation WHERE target_ref = ?1 ORDER BY id LIMIT ?2;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(user_, "prepare list_annotations_for_target");
  }
  sqlite3_bind_text(stmt, 1, target_ref.data(),
                    static_cast<int>(target_ref.size()), SQLITE_STATIC);
  sqlite3_bind_int(stmt, 2, limit);
  std::vector<AnnotationRow> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    AnnotationRow r;
    r.id = sqlite3_column_int64(stmt, 0);
    r.target_ref =
        reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    r.kind = sqlite3_column_int(stmt, 2);
    if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) {
      r.body = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    }
    if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) {
      const auto* p =
          static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt, 4));
      const int n = sqlite3_column_bytes(stmt, 4);
      r.geom.assign(p, p + n);
    }
    r.created_at = sqlite3_column_int64(stmt, 5);
    r.updated_at = sqlite3_column_int64(stmt, 6);
    out.push_back(std::move(r));
  }
  sqlite3_finalize(stmt);
  return out;
}

void Store::put_http_body(std::string_view url,
                          std::span<const std::uint8_t> data,
                          std::optional<std::int64_t> blob_id,
                          std::optional<std::int64_t> fetched_at) {
  if (url.empty()) {
    throw std::invalid_argument("put_http_body: empty url");
  }
  if (data.empty()) {
    throw std::invalid_argument("put_http_body: empty data");
  }
  const std::int64_t at = fetched_at ? *fetched_at : now_unix_s();
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          bulk_,
          "INSERT INTO http_body(url, fetched_at, blob_id, data) "
          "VALUES(?1,?2,?3,?4) "
          "ON CONFLICT(url) DO UPDATE SET fetched_at = excluded.fetched_at, "
          "blob_id = excluded.blob_id, data = excluded.data;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(bulk_, "prepare put_http_body");
  }
  sqlite3_bind_text(stmt, 1, url.data(), static_cast<int>(url.size()),
                    SQLITE_STATIC);
  sqlite3_bind_int64(stmt, 2, at);
  if (blob_id) {
    sqlite3_bind_int64(stmt, 3, *blob_id);
  } else {
    sqlite3_bind_null(stmt, 3);
  }
  sqlite3_bind_blob(stmt, 4, data.data(), static_cast<int>(data.size()),
                    SQLITE_STATIC);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(bulk_, "step put_http_body");
  }
  sqlite3_finalize(stmt);
}

std::optional<Store::HttpBodyRow> Store::get_http_body(
    std::string_view url) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          bulk_,
          "SELECT url, fetched_at, blob_id, data FROM http_body WHERE url = ?1;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(bulk_, "prepare get_http_body");
  }
  sqlite3_bind_text(stmt, 1, url.data(), static_cast<int>(url.size()),
                    SQLITE_STATIC);
  std::optional<HttpBodyRow> out;
  if (sqlite3_step(stmt) == SQLITE_ROW) {
    HttpBodyRow r;
    r.url = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    r.fetched_at = sqlite3_column_int64(stmt, 1);
    if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) {
      r.blob_id = sqlite3_column_int64(stmt, 2);
    }
    const auto* p =
        static_cast<const std::uint8_t*>(sqlite3_column_blob(stmt, 3));
    const int n = sqlite3_column_bytes(stmt, 3);
    r.data.assign(p, p + n);
    out = std::move(r);
  }
  sqlite3_finalize(stmt);
  return out;
}

bool Store::delete_http_body(std::string_view url) {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(bulk_, "DELETE FROM http_body WHERE url = ?1;", -1,
                         &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(bulk_, "prepare delete_http_body");
  }
  sqlite3_bind_text(stmt, 1, url.data(), static_cast<int>(url.size()),
                    SQLITE_STATIC);
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    sqlite3_finalize(stmt);
    throw_sqlite(bulk_, "step delete_http_body");
  }
  const int changes = sqlite3_changes(bulk_);
  sqlite3_finalize(stmt);
  return changes > 0;
}

std::vector<Store::TileRow> Store::list_tiles_for_region(std::int64_t media_id,
                                                        std::int64_t region_id,
                                                        int limit) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "SELECT media_id, region_id, scale, x, y, width, height, codec_id, "
          "quality FROM tile WHERE media_id = ?1 AND region_id = ?2 "
          "ORDER BY scale, y, x LIMIT ?3;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare list_tiles_for_region");
  }
  sqlite3_bind_int64(stmt, 1, media_id);
  sqlite3_bind_int64(stmt, 2, region_id);
  sqlite3_bind_int(stmt, 3, limit);
  std::vector<TileRow> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    TileRow r;
    r.media_id = sqlite3_column_int64(stmt, 0);
    r.region_id = sqlite3_column_int64(stmt, 1);
    r.scale = sqlite3_column_int(stmt, 2);
    r.x = sqlite3_column_int(stmt, 3);
    r.y = sqlite3_column_int(stmt, 4);
    r.width = sqlite3_column_int(stmt, 5);
    r.height = sqlite3_column_int(stmt, 6);
    r.codec_id = static_cast<CodecId>(sqlite3_column_int(stmt, 7));
    if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) {
      r.quality = sqlite3_column_int(stmt, 8);
    }
    out.push_back(std::move(r));
  }
  sqlite3_finalize(stmt);
  return out;
}

std::vector<int> Store::list_tile_scales(std::int64_t media_id,
                                         std::int64_t region_id) const {
  sqlite3_stmt* stmt = nullptr;
  if (sqlite3_prepare_v2(
          index_,
          "SELECT DISTINCT scale FROM tile WHERE media_id = ?1 AND "
          "region_id = ?2 ORDER BY scale;",
          -1, &stmt, nullptr) != SQLITE_OK) {
    throw_sqlite(index_, "prepare list_tile_scales");
  }
  sqlite3_bind_int64(stmt, 1, media_id);
  sqlite3_bind_int64(stmt, 2, region_id);
  std::vector<int> out;
  while (sqlite3_step(stmt) == SQLITE_ROW) {
    out.push_back(sqlite3_column_int(stmt, 0));
  }
  sqlite3_finalize(stmt);
  return out;
}

}  // namespace thumtoo
