// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

struct sqlite3;

namespace thumtoo {

/// Hash algorithm ids seeded in index.hash_algo (stable across runs).
enum class HashAlgoId : int {
  Sha256 = 1,
  Sha1 = 2,
  Md5 = 3,
  Crc32 = 4,
};

/// Codec ids seeded in index.codec.
enum class CodecId : int {
  Jpeg = 1,
  Jxl = 2,
  Png = 3,
};

/// Index DB status values for blob.status (extensible).
enum class BlobStatus : int {
  Unknown = 0,
  Ok = 1,
  Missing = 2,
  Error = 3,
};

/// media.kind values (docs/DATABASE.md).
enum class MediaKind : int {
  Unknown = 0,
  Image = 1,
  Document = 2,
  Video = 3,
  Audio = 4,
};

/// region.kind values.
enum class RegionKind : int {
  Full = 0,     ///< Single-frame image (mandatory for image media)
  Page = 1,     ///< Document page (key = 1-based page number string)
  Fragment = 2, ///< e.g. HTML id (future)
  TimeRange = 3,
};

/// media.status / similar.
enum class MediaStatus : int {
  Unknown = 0,
  Ready = 1,
  Pending = 2,
  Error = 3,
  Unsupported = 4,
};

/// Redesign index/bulk/user stores (docs/DATABASE.md, docs/PLAN.md).
///
/// Redesign Store spine (schema ≥ 100). Replaces schema-4 Database + BlobStore.
/// integer blob ids, blob_hash digests, locators, media/region/tiles.
/// Not thread-safe; one writer discipline at a higher layer.
class Store {
 public:
  Store() = default;
  Store(const Store&) = delete;
  Store& operator=(const Store&) = delete;
  Store(Store&&) noexcept;
  Store& operator=(Store&&) noexcept;
  ~Store();

  struct Paths {
    std::filesystem::path cache_root;  // index + bulk
    std::filesystem::path data_root;   // user (may equal cache_root for tests)
  };

  /// Open or create the three SQLite files. If index exists with an unsupported
  /// or legacy (pre-redesign) schema_version, it is removed and recreated; user
  /// DB is never deleted by this path.
  ///
  /// Layout:
  ///   cache_root/index.sqlite
  ///   cache_root/bulk.sqlite
  ///   data_root/user.sqlite
  static Store open(const Paths& paths);

  /// Convenience: index+bulk under cache_root; user.sqlite also under
  /// cache_root (tests only). Production: Client::open uses default_data_root().
  static Store open(const std::filesystem::path& cache_root);

  /// Ephemeral index/bulk/user as three independent SQLite `:memory:` DBs.
  /// Nothing is written to disk; process exit discards all rows. For cold
  /// probe benchmarks (`thumtoo-prepare --no-cache`) without Store I/O.
  static Store open_memory();

  [[nodiscard]] const std::filesystem::path& cache_root() const {
    return cache_root_;
  }
  [[nodiscard]] const std::filesystem::path& data_root() const {
    return data_root_;
  }
  [[nodiscard]] int index_schema_version() const { return index_schema_version_; }
  [[nodiscard]] bool opened() const { return index_ != nullptr; }

  // --- schema_meta on index ---
  [[nodiscard]] std::optional<std::string> meta_get(std::string_view key) const;
  void meta_set(std::string_view key, std::string_view value);

  // --- blob ---
  struct BlobRow {
    std::int64_t id = 0;
    std::optional<std::int64_t> size;
    BlobStatus status = BlobStatus::Unknown;
    std::int64_t created_at = 0;
    std::int64_t updated_at = 0;
  };

  /// Insert a new blob row; returns id.
  [[nodiscard]] std::int64_t insert_blob(std::optional<std::int64_t> size,
                                         BlobStatus status = BlobStatus::Unknown);

  [[nodiscard]] std::optional<BlobRow> find_blob(std::int64_t id) const;
  void set_blob_size(std::int64_t id, std::int64_t size);
  void set_blob_status(std::int64_t id, BlobStatus status);

  // --- blob_lqip (durable ThumbHash/Handsum placeholder; keyed by blob) ---
  /// page_1based: 0 = whole-blob / still image; N = document page.
  void put_blob_lqip(std::int64_t blob_id, int kind,
                     std::span<const std::uint8_t> data, int page_1based = 0);
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> get_blob_lqip(
      std::int64_t blob_id, int page_1based = 0) const;
  [[nodiscard]] std::optional<int> get_blob_lqip_kind(std::int64_t blob_id,
                                                      int page_1based = 0) const;

  // --- page text / document outline (serialized payloads; keyed by blob) ---
  void put_page_text_layer(std::int64_t blob_id, int page_1based,
                           std::string_view layout_key,
                           std::span<const std::uint8_t> data);
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> get_page_text_layer(
      std::int64_t blob_id, int page_1based,
      std::string_view layout_key) const;

  /** Delete all layout_key variants (native + OCR) for one page. Returns rows. */
  std::int64_t delete_page_text_layers(std::int64_t blob_id, int page_1based);
  /** Delete every page_text_layer row for a blob. Returns rows. */
  std::int64_t delete_page_text_layers_for_blob(std::int64_t blob_id);


  void put_document_outline(std::int64_t blob_id, std::string_view layout_key,
                            std::span<const std::uint8_t> data);
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> get_document_outline(
      std::int64_t blob_id, std::string_view layout_key) const;

  // --- blob_hash ---
  /// Store digest (raw bytes). Replaces existing digest for (blob, algo).
  void put_hash(std::int64_t blob_id, HashAlgoId algo,
                std::span<const std::uint8_t> digest);

  /// Lookup blob id by algorithm + raw digest.
  [[nodiscard]] std::optional<std::int64_t> find_blob_by_hash(
      HashAlgoId algo, std::span<const std::uint8_t> digest) const;

  [[nodiscard]] std::optional<std::vector<std::uint8_t>> get_hash(
      std::int64_t blob_id, HashAlgoId algo) const;

  /// Canonical public ref when SHA-256 is known: "blob:sha256:" + lowercase hex.
  [[nodiscard]] std::optional<std::string> blob_ref_sha256(
      std::int64_t blob_id) const;

  /// Parse "blob:sha256:<hex>" or bare 64-hex → raw 32 bytes; nullopt if invalid.
  [[nodiscard]] static std::optional<std::vector<std::uint8_t>> parse_sha256_digest(
      std::string_view ref_or_hex);

  /// Format raw 32-byte SHA-256 as "blob:sha256:" + hex.
  [[nodiscard]] static std::string format_blob_ref_sha256(
      std::span<const std::uint8_t> digest32);

  // --- locator ---
  struct LocatorRow {
    std::int64_t id = 0;
    std::string uri;
    std::optional<std::int64_t> blob_id;
    std::optional<std::int64_t> size;
    std::optional<std::int64_t> mtime_ns;
    std::optional<std::string> outer_path;
    std::optional<std::string> member_path;
    std::int64_t updated_at = 0;
  };

  /// Insert or update by uri (unique). Returns locator id.
  std::int64_t upsert_locator(std::string_view uri,
                              std::optional<std::int64_t> blob_id,
                              std::optional<std::int64_t> size,
                              std::optional<std::int64_t> mtime_ns,
                              std::optional<std::string> outer_path = std::nullopt,
                              std::optional<std::string> member_path = std::nullopt);

  void bind_locator_blob(std::string_view uri, std::int64_t blob_id);

  [[nodiscard]] std::optional<LocatorRow> find_locator(std::string_view uri) const;
  [[nodiscard]] std::vector<LocatorRow> list_locators(int limit = 100) const;
  [[nodiscard]] std::vector<LocatorRow> list_locators_by_uri_prefix(
      std::string_view uri_prefix, int limit = 100) const;
  /// SQL LIKE on uri (caller supplies pattern; % and _ wildcards).
  [[nodiscard]] std::vector<LocatorRow> list_locators_like(
      std::string_view uri_like_pattern, int limit = 100) const;
  [[nodiscard]] std::vector<LocatorRow> list_locators_by_outer_path_prefix(
      std::string_view path_prefix, int limit = 100) const;
  [[nodiscard]] std::vector<LocatorRow> list_locators_for_blob(
      std::int64_t blob_id, int limit = 100) const;

  /// Delete locator by URI. Returns bound blob_id if any (for orphan check).
  [[nodiscard]] std::optional<std::int64_t> delete_locator(std::string_view uri);

  /// Delete locators whose uri starts with prefix (literal prefix, not SQL LIKE).
  [[nodiscard]] std::int64_t delete_locators_with_uri_prefix(
      std::string_view uri_prefix);

  /// If no locators reference blob_id, delete media/tiles/hash/blob (and bulk
  /// tile payloads). Returns number of tile_blob rows removed from bulk.
  [[nodiscard]] std::int64_t purge_blob_if_unreferenced(std::int64_t blob_id);

  /// Forget one URI: delete locator, then purge blob if unreferenced.
  struct ForgetStats {
    bool locator_removed = false;
    bool blob_purged = false;
    std::int64_t tiles_deleted = 0;
  };
  [[nodiscard]] ForgetStats forget_uri(std::string_view uri, bool dry_run = false);

  /// Forget every locator whose uri starts with prefix; then purge orphan blobs
  /// that become unreferenced. Useful for directory/file:// tree clears.
  struct PrefixForgetStats {
    std::int64_t locators_removed = 0;
    std::int64_t blobs_purged = 0;
    std::int64_t tiles_deleted = 0;
  };
  [[nodiscard]] PrefixForgetStats forget_uri_prefix(std::string_view uri_prefix,
                                                    bool dry_run = false);

  /// Blob ids with no locator rows (after forget_uri or partial deletes).
  [[nodiscard]] std::vector<std::int64_t> list_orphan_blob_ids(
      int limit = 100000) const;

  /// Purge every orphan blob (tiles + media + blob). Returns blobs purged and
  /// total tile_blob rows deleted.
  struct OrphanPurgeStats {
    std::int64_t blobs_purged = 0;
    std::int64_t tiles_deleted = 0;
  };
  [[nodiscard]] OrphanPurgeStats purge_orphan_blobs(bool dry_run = false,
                                                     int limit = 100000);

  [[nodiscard]] std::int64_t count_blobs() const;
  [[nodiscard]] std::int64_t count_locators() const;

  // --- media ---
  struct MediaRow {
    std::int64_t id = 0;
    std::int64_t blob_id = 0;
    MediaKind kind = MediaKind::Unknown;
    std::optional<int> width;
    std::optional<int> height;
    std::optional<std::int64_t> duration_ms;
    std::optional<int> page_count;
    std::optional<int> still_count;
    MediaStatus status = MediaStatus::Unknown;
    std::optional<std::string> error_code;
    std::int64_t updated_at = 0;
  };

  /// Insert media for blob+kind. Fails if (blob_id, kind) already exists.
  [[nodiscard]] std::int64_t insert_media(std::int64_t blob_id, MediaKind kind,
                                          std::optional<int> width = {},
                                          std::optional<int> height = {},
                                          MediaStatus status = MediaStatus::Unknown);

  /// Ensure image media + full region exist for blob; returns media_id.
  /// Creates media(kind=image) and region(kind=full, key="") if missing.
  [[nodiscard]] std::int64_t ensure_image_media(std::int64_t blob_id,
                                                std::optional<int> width = {},
                                                std::optional<int> height = {});

  [[nodiscard]] std::optional<MediaRow> find_media(std::int64_t media_id) const;
  [[nodiscard]] std::optional<MediaRow> find_media_for_blob(
      std::int64_t blob_id, MediaKind kind) const;

  void set_media_size(std::int64_t media_id, int width, int height);
  /// Per-page layout size from the document (PDF/DjVu/EPUB). Not media size.
  void set_region_size(std::int64_t region_id, int width, int height);
  void set_media_page_count(std::int64_t media_id, int page_count);
  void set_media_status(std::int64_t media_id, MediaStatus status,
                        std::optional<std::string_view> error_code = {});

  // --- region ---
  struct RegionRow {
    std::int64_t id = 0;
    std::int64_t media_id = 0;
    RegionKind kind = RegionKind::Full;
    std::string key;
    std::optional<int> ordinal;
    /// Page layout size from the document (one size: PDF/DjVu/EPUB layout).
    std::optional<int> width;
    std::optional<int> height;
  };

  [[nodiscard]] std::int64_t insert_region(std::int64_t media_id, RegionKind kind,
                                           std::string_view key,
                                           std::optional<int> ordinal = {});

  /// Find or create region; returns region_id.
  [[nodiscard]] std::int64_t ensure_region(std::int64_t media_id, RegionKind kind,
                                           std::string_view key,
                                           std::optional<int> ordinal = {});

  [[nodiscard]] std::optional<RegionRow> find_region(std::int64_t region_id) const;
  [[nodiscard]] std::optional<RegionRow> find_region_by_key(
      std::int64_t media_id, RegionKind kind, std::string_view key) const;

  /// Full region for image media (kind=full, key empty).
  [[nodiscard]] std::optional<RegionRow> find_full_region(
      std::int64_t media_id) const;

  // --- tile (index metadata + bulk payload) ---
  struct TileRow {
    std::int64_t media_id = 0;
    std::int64_t region_id = 0;
    int scale = 0;
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
    CodecId codec_id = CodecId::Jpeg;
    std::optional<int> quality;
  };

  /// Upsert tile metadata (index) and payload (bulk).
  void put_tile(const TileRow& meta, std::span<const std::uint8_t> data);

  [[nodiscard]] bool has_tile(std::int64_t media_id, std::int64_t region_id,
                              int scale, int x, int y) const;

  /// Index metadata only (no payload).
  [[nodiscard]] std::optional<TileRow> find_tile_meta(std::int64_t media_id,
                                                     std::int64_t region_id,
                                                     int scale, int x,
                                                     int y) const;

  /// Payload from bulk DB; nullopt if missing.
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> get_tile_data(
      std::int64_t media_id, std::int64_t region_id, int scale, int x,
      int y) const;

  void delete_tiles_for_region(std::int64_t media_id, std::int64_t region_id);

  [[nodiscard]] std::int64_t count_tiles() const;
  [[nodiscard]] std::int64_t count_media() const;
  [[nodiscard]] std::int64_t count_regions() const;

  // --- container_member (archive TOC; hash only on full read / tag / explicit) ---
  struct ContainerMemberRow {
    std::int64_t container_id = 0;
    std::string member_path;
    bool is_directory = false;
    std::optional<std::int64_t> uncompressed_size;
    std::optional<std::int64_t> blob_id;  // set when member bytes are hashed
  };

  /// Upsert one TOC entry. Does not hash; blob_id only set if provided.
  void upsert_container_member(std::int64_t container_id,
                               std::string_view member_path, bool is_directory,
                               std::optional<std::int64_t> uncompressed_size,
                               std::optional<std::int64_t> blob_id = {});

  /// Replace all members for a container (transactional). Used on TOC refresh.
  void replace_container_members(std::int64_t container_id,
                                 const std::vector<ContainerMemberRow>& members);

  [[nodiscard]] std::vector<ContainerMemberRow> list_container_members(
      std::int64_t container_id, int limit = 100000) const;

  [[nodiscard]] std::optional<ContainerMemberRow> find_container_member(
      std::int64_t container_id, std::string_view member_path) const;

  /// After hashing member bytes: attach blob_id to the TOC row.
  void set_container_member_blob(std::int64_t container_id,
                                 std::string_view member_path,
                                 std::int64_t member_blob_id);

  [[nodiscard]] std::int64_t count_container_members(
      std::int64_t container_id) const;

  // --- documents (page regions; tiles optional until rasterized) ---
  /// Ensure document media for blob; optionally set page_count.
  [[nodiscard]] std::int64_t ensure_document_media(
      std::int64_t blob_id, std::optional<int> page_count = {});

  /// Ensure region for 1-based page number.
  /// key = decimal page, or epub_page_region_key(page, layout) when region_key set.
  [[nodiscard]] std::int64_t ensure_page_region(std::int64_t media_id,
                                               int page_1based);
  /// Full region key (PDF/DjVu: "N"; EPUB: "N|layout_key").
  [[nodiscard]] std::int64_t ensure_page_region(std::int64_t media_id,
                                               int page_1based,
                                               std::string_view region_key);

  // --- directory_snapshot (cache-first folder open; index DB) ---
  struct DirectorySnapshotRow {
    std::string dir_uri;
    std::optional<std::int64_t> size;
    std::optional<std::int64_t> mtime_ns;
    std::int64_t listed_at = 0;
    bool incomplete = false;
    // Schema ≥101 (optional / NULL on legacy rows)
    std::optional<std::int64_t> inode;
    std::optional<std::int64_t> dev;
    std::optional<std::int64_t> nlink;
    std::optional<std::int64_t> mode;
    std::optional<std::int64_t> uid;
    std::optional<std::int64_t> gid;
    std::optional<std::string> error_code;
  };

  struct DirectoryEntryRow {
    std::string dir_uri;
    std::string name;
    std::optional<std::string> child_uri;
    bool is_dir = false;
    std::optional<std::int64_t> size;
    std::optional<std::int64_t> mtime_ns;
    // Schema ≥101 (optional / NULL on legacy rows)
    std::optional<std::int64_t> file_type;  // kFsFileType*
    std::optional<std::int64_t> mode;
    std::optional<std::int64_t> uid;
    std::optional<std::int64_t> gid;
    std::optional<std::int64_t> atime_ns;
    std::optional<std::int64_t> ctime_ns;
    std::optional<std::int64_t> birth_ns;
    std::optional<std::int64_t> nlink;
    std::optional<std::int64_t> inode;
    std::optional<std::int64_t> dev;
    std::optional<std::int64_t> rdev;
    std::optional<std::string> symlink_target;
    std::optional<std::int64_t> blob_id;
    std::optional<std::int64_t> flags;
  };

  /// Replace snapshot meta + all entries transactionally (TOC-style refresh).
  void replace_directory_snapshot(const DirectorySnapshotRow& snap,
                                  const std::vector<DirectoryEntryRow>& entries);

  [[nodiscard]] std::optional<DirectorySnapshotRow> find_directory_snapshot(
      std::string_view dir_uri) const;

  [[nodiscard]] std::vector<DirectoryEntryRow> list_directory_entries(
      std::string_view dir_uri, int limit = 100000) const;

  void delete_directory_snapshot(std::string_view dir_uri);

  [[nodiscard]] std::int64_t count_directory_snapshots() const;

  // --- user tags (tag_def + blob_tag; survive index/bulk wipe) ---
  struct TagDefRow {
    std::int64_t id = 0;
    std::optional<std::string> uuid;  // stable export key; schema ≥101
    std::string name;
    std::optional<std::string> label;
    std::optional<std::string> color;
    std::optional<std::string> badge;
    std::int64_t created_at = 0;
  };

  /// Ensure a tag_def row by unique name; returns id. Optional meta only applied
  /// on insert (existing rows keep their label/color/badge).
  [[nodiscard]] std::int64_t ensure_tag_def(
      std::string_view name, std::optional<std::string_view> label = {},
      std::optional<std::string_view> color = {},
      std::optional<std::string_view> badge = {});

  [[nodiscard]] std::optional<TagDefRow> find_tag_def(std::int64_t id) const;
  [[nodiscard]] std::optional<TagDefRow> find_tag_def_by_name(
      std::string_view name) const;

  /// Attach tag to blob_ref (`blob:sha256:…`). Creates tag_def if needed.
  /// Empty name or blob_ref is a no-op.
  void add_blob_tag(std::string_view blob_ref, std::string_view tag_name,
                    std::string_view source = "user");

  /// Returns true if a row was removed.
  bool remove_blob_tag(std::string_view blob_ref, std::string_view tag_name);

  [[nodiscard]] std::vector<std::string> tags_for_blob_ref(
      std::string_view blob_ref) const;

  [[nodiscard]] std::vector<std::string> blob_refs_for_tag(
      std::string_view tag_name, int limit = 1000) const;

  // --- user collections / sets (dirtoo FileSet direction) ---
  struct CollectionRow {
    std::int64_t id = 0;
    std::optional<std::string> uuid;  // stable export key; schema ≥101
    std::optional<std::string> label;
    std::optional<std::string> color;
    std::int64_t created_at = 0;
    std::int64_t updated_at = 0;
  };

  struct CollectionMemberRow {
    std::int64_t collection_id = 0;
    std::string blob_ref;
    std::optional<int> ordinal;
    std::optional<std::string> path_key;
  };

  [[nodiscard]] std::int64_t create_collection(
      std::optional<std::string_view> label = {},
      std::optional<std::string_view> color = {});

  [[nodiscard]] std::optional<CollectionRow> find_collection(
      std::int64_t id) const;

  void set_collection_label(std::int64_t id, std::optional<std::string_view> label);
  void set_collection_color(std::int64_t id, std::optional<std::string_view> color);

  /// Insert or replace membership (PRIMARY KEY collection_id + blob_ref).
  void upsert_collection_member(std::int64_t collection_id,
                                std::string_view blob_ref,
                                std::optional<int> ordinal = {},
                                std::optional<std::string_view> path_key = {});

  bool remove_collection_member(std::int64_t collection_id,
                                std::string_view blob_ref);

  [[nodiscard]] std::vector<CollectionMemberRow> list_collection_members(
      std::int64_t collection_id, int limit = 100000) const;

  /// CASCADE deletes members.
  void delete_collection(std::int64_t id);

  [[nodiscard]] std::int64_t count_collections() const;

  // --- user bookmarks ---
  struct BookmarkRow {
    std::int64_t id = 0;
    std::string target_ref;
    std::optional<std::string> title;
    std::int64_t created_at = 0;
    std::int64_t updated_at = 0;
  };

  [[nodiscard]] std::int64_t create_bookmark(
      std::string_view target_ref, std::optional<std::string_view> title = {});

  [[nodiscard]] std::optional<BookmarkRow> find_bookmark(std::int64_t id) const;

  void set_bookmark_title(std::int64_t id, std::optional<std::string_view> title);

  void delete_bookmark(std::int64_t id);

  [[nodiscard]] std::vector<BookmarkRow> list_bookmarks_for_target(
      std::string_view target_ref, int limit = 1000) const;

  [[nodiscard]] std::int64_t count_bookmarks() const;

  // --- user link edges (trails; source defaults to 2 = user) ---
  struct LinkEdgeRow {
    std::int64_t id = 0;
    std::string from_ref;
    std::string to_ref;
    std::optional<std::string> rel;
    int source = 2;  // 2 = user (docs/DATABASE.md)
    std::int64_t created_at = 0;
  };

  /// Insert or ignore on UNIQUE (from_ref, to_ref, rel, source). Returns id of
  /// existing or new row.
  [[nodiscard]] std::int64_t add_link_edge(std::string_view from_ref,
                                           std::string_view to_ref,
                                           std::optional<std::string_view> rel = {},
                                           int source = 2);

  bool remove_link_edge(std::int64_t id);

  [[nodiscard]] std::vector<LinkEdgeRow> list_links_from(
      std::string_view from_ref, int limit = 1000) const;

  [[nodiscard]] std::vector<LinkEdgeRow> list_links_to(std::string_view to_ref,
                                                      int limit = 1000) const;

  // --- user annotations (highlights / notes; geom optional) ---
  struct AnnotationRow {
    std::int64_t id = 0;
    std::string target_ref;
    int kind = 0;
    std::optional<std::string> body;
    std::vector<std::uint8_t> geom;  // empty if none
    std::int64_t created_at = 0;
    std::int64_t updated_at = 0;
  };

  [[nodiscard]] std::int64_t create_annotation(
      std::string_view target_ref, int kind,
      std::optional<std::string_view> body = {},
      std::span<const std::uint8_t> geom = {});

  [[nodiscard]] std::optional<AnnotationRow> find_annotation(
      std::int64_t id) const;

  void set_annotation_body(std::int64_t id,
                           std::optional<std::string_view> body);
  void set_annotation_geom(std::int64_t id, std::span<const std::uint8_t> geom);

  void delete_annotation(std::int64_t id);

  [[nodiscard]] std::vector<AnnotationRow> list_annotations_for_target(
      std::string_view target_ref, int limit = 1000) const;

  // --- bulk http_body (optional HTTPS cache; Phase D §12) ---
  struct HttpBodyRow {
    std::string url;
    std::int64_t fetched_at = 0;
    std::optional<std::int64_t> blob_id;
    std::vector<std::uint8_t> data;
  };

  void put_http_body(std::string_view url, std::span<const std::uint8_t> data,
                     std::optional<std::int64_t> blob_id = {},
                     std::optional<std::int64_t> fetched_at = {});

  [[nodiscard]] std::optional<HttpBodyRow> get_http_body(
      std::string_view url) const;

  bool delete_http_body(std::string_view url);

  // --- tile listing (for assemble / GC) ---
  [[nodiscard]] std::vector<TileRow> list_tiles_for_region(
      std::int64_t media_id, std::int64_t region_id, int limit = 100000) const;

  /// Distinct scales present for a region (ascending).
  [[nodiscard]] std::vector<int> list_tile_scales(std::int64_t media_id,
                                                  std::int64_t region_id) const;

 private:
  Store(sqlite3* index, sqlite3* bulk, sqlite3* user,
        std::filesystem::path cache_root, std::filesystem::path data_root,
        int index_schema_version);

  void exec_index(const char* sql) const;
  void exec_bulk(const char* sql) const;
  void exec_user(const char* sql) const;
  void migrate_or_init_index();
  void ensure_optional_index_tables();
  void migrate_or_init_bulk();
  void migrate_or_init_user();
  void seed_lookups();

  static sqlite3* open_sqlite(const std::filesystem::path& path);
  static void close_sqlite(sqlite3*& db);

  sqlite3* index_ = nullptr;
  sqlite3* bulk_ = nullptr;
  sqlite3* user_ = nullptr;
  std::filesystem::path cache_root_;
  std::filesystem::path data_root_;
  int index_schema_version_ = 0;
};

}  // namespace thumtoo
