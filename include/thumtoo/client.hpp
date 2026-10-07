// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/archive.hpp"
#include "thumtoo/executor.hpp"
#include "thumtoo/store.hpp"
#include "thumtoo/types.hpp"
#include "thumtoo/activity.hpp"
#include "thumtoo/text.hpp"
#include "thumtoo/ocr.hpp"
#include "thumtoo/pdf.hpp"
#include "thumtoo/epub.hpp"
#include "thumtoo/djvu.hpp"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <thread>
#include <unordered_map>
#include <deque>
#include <list>
#include <vector>

namespace thumtoo {

/// Host feature probes (biltoo may #ifdef these when linking older trees).
#ifndef THUMTOO_API_INTEREST_EPOCH
#define THUMTOO_API_INTEREST_EPOCH 1
#endif
#ifndef THUMTOO_API_OVERVIEW_PIXELS
#define THUMTOO_API_OVERVIEW_PIXELS 1
#endif
#ifndef THUMTOO_API_SET_INTEREST
#define THUMTOO_API_SET_INTEREST 1
#endif
#ifndef THUMTOO_API_DOCUMENT_INDEX
#define THUMTOO_API_DOCUMENT_INDEX 1
#endif
#ifndef THUMTOO_API_REQUEST_RASTER
#define THUMTOO_API_REQUEST_RASTER 1
#endif
#ifndef THUMTOO_API_FULL_PIXELS
#define THUMTOO_API_FULL_PIXELS 1
#endif
#ifndef THUMTOO_API_PURGE_URI
#define THUMTOO_API_PURGE_URI 1
#endif
#ifndef THUMTOO_API_STORE
#define THUMTOO_API_STORE 1
#endif

/// In-process client: cache-only get_* + async request_* (DESIGN API sketch).
///
/// Threading: get_* are non-blocking SQLite reads and are intended for the GUI
/// thread. request_* enqueue work onto a pool of worker threads (default:
/// hardware_concurrency). Completion callbacks are delivered via the Executor
/// installed at open() — never directly from a worker unless the Executor runs
/// inline (CLI default). SQLite is used in WAL mode with a busy timeout so
/// concurrent workers can share the connection.
class Client {
 public:
  /// Cache locator (path/URI → content identity on Store).
  struct LocatorRow {
    std::string uri;
    std::optional<std::string> content_id;
    std::optional<std::string> outer_path;
    std::optional<std::string> member_path;
    std::optional<std::int64_t> size;
    std::optional<std::int64_t> mtime_ns;
  };

  /// Archive TOC row (container member listing).
  struct ArchiveEntryRow {
    std::string archive_uri;
    std::string member_path;
    std::optional<std::int64_t> uncompressed_size;
  };

  /// Result of purge_uri / purge_path (Store forget_uri).
  struct PurgeStats {
    std::vector<std::string> removed_uris;
    std::vector<std::string> purged_content_ids;
    std::int64_t tiles_deleted = 0;
    std::int64_t levels_deleted = 0;  // locator count when used by purge_uri_prefix
  };

  using SizeCallback = std::function<void(std::string uri, SizeReply)>;
  using PixelsCallback =
      std::function<void(std::string uri, int max_edge, std::optional<PixelLevel>)>;
  using TileCallback = std::function<void(std::string uri, int scale, int x, int y,
                                          std::optional<TileBlob>)>;

  Client() = default;
  Client(const Client&) = delete;
  Client& operator=(const Client&) = delete;
  Client(Client&&) = delete;
  Client& operator=(Client&&) = delete;
  ~Client();

  /// \param worker_threads 0 → std::thread::hardware_concurrency() (min 1, max 32).
  /// \param data_root user.sqlite root (tags/collections). Empty →
  ///        `default_data_root()` (`$XDG_STATE_HOME/thumtoo`, not cache).
  ///        Store index/blobs live at `cache_root/` (HOST_CUTOVER.md).
  static std::unique_ptr<Client> open(const std::filesystem::path& cache_root,
                                      Executor executor = {},
                                      unsigned worker_threads = 0,
                                      const std::filesystem::path& data_root = {});

  /// Ephemeral Store (`:memory:` SQLite). No disk cache; for size-probe
  /// benchmarks without durable Store I/O (`thumtoo-prepare --no-cache`).
  static std::unique_ptr<Client> open_memory(Executor executor = {},
                                             unsigned worker_threads = 0);

  /// Redesign index/bulk/user (Store-only).
  [[nodiscard]] Store& store() { return *store_; }
  [[nodiscard]] const Store& store() const { return *store_; }

  /// Cache-only size from Store region/media dims.
  /// Does not open source files, list tiles, or load LQIP/EMB.
  [[nodiscard]] std::optional<Size> get_size(std::string_view uri) const;
  [[nodiscard]] std::optional<ContentMeta> get_meta(std::string_view uri) const;

  // --- directory snapshot (Store; cache-first folder open) ---
  using DirectorySnapshotRow = Store::DirectorySnapshotRow;
  using DirectoryEntryRow = Store::DirectoryEntryRow;

  void replace_directory_snapshot(
      const DirectorySnapshotRow& snap,
      const std::vector<DirectoryEntryRow>& entries);
  [[nodiscard]] std::optional<DirectorySnapshotRow> find_directory_snapshot(
      std::string_view dir_uri) const;
  [[nodiscard]] std::vector<DirectoryEntryRow> list_directory_entries(
      std::string_view dir_uri, int limit = 100000) const;
  void delete_directory_snapshot(std::string_view dir_uri);

  /// Walk `dir_path` on the filesystem and write a Store snapshot for the
  /// corresponding `file://` URI. Returns entry count; no-op without Store.
  [[nodiscard]] std::size_t refresh_directory_snapshot(
      const std::filesystem::path& dir_path);

  /// Cache-only: locator rows known to this cache (browse without source I/O).
  [[nodiscard]] std::vector<LocatorRow> list_locators(int limit = 100) const;
  [[nodiscard]] std::optional<LocatorRow> find_locator(
      std::string_view uri) const;

  /// Cache-only listing helpers (prefix/LIKE; empty until Store listing lands).
  [[nodiscard]] std::vector<LocatorRow> list_locators_by_uri_prefix(
      std::string_view uri_prefix, int limit = 100) const;
  [[nodiscard]] std::vector<LocatorRow> list_locators_by_outer_path_prefix(
      std::string_view path_prefix, int limit = 100) const;
  [[nodiscard]] std::vector<LocatorRow> list_locators_like(
      std::string_view uri_like_pattern, int limit = 100) const;

  /// Resolve a location or content-id URI to the durable content_id (cache only).
  /// Returns nullopt if the locator is unknown or not yet hashed.
  [[nodiscard]] std::optional<std::string> resolve_content_id(
      std::string_view uri) const;

  /// Locators that share this content_id (same bytes, different paths).
  [[nodiscard]] std::vector<LocatorRow> list_uris_for_content_id(
      std::string_view content_id, int limit = 100) const;

  /// Cache-only meta by content_id (same as get_meta("sha256:…")).
  [[nodiscard]] std::optional<ContentMeta> get_meta_for_content_id(
      std::string_view content_id) const;

  /// Load original media bytes for a location or content-id URI.
  /// **Source I/O** (not cache-only): regular files and archive members.
  /// Content-id tries each known locator. PDF pages return nullopt.
  /// http(s) uses libcurl when THUMTOO_HAVE_CURL is enabled.
  /// Rejects payloads larger than kArchiveMaxMemberUncompressedBytes.
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> read_source_bytes(
      std::string_view uri_or_content_id);

  /// Cache-only: best stored **soft** preview with long-edge ≤ max_edge
  /// (frame 0 default). Does not build levels. Soft storage is capped at
  /// kMaxSoftLadderEdge (512); a 256-level is returned for larger max_edge
  /// until a higher soft level has been ensured.
  [[nodiscard]] std::optional<PixelLevel> get_pixels(
      std::string_view uri, int max_edge, int frame_idx = 0,
      bool allow_tile_synth = true) const;

  /**
   * Cache-only: build a full-frame preview from stored grid tiles.
   * Picks a pyramid scale whose long edge is ≥ min(max_edge, native), requires
   * every cell at that scale to be present, composites, then shrinks to
   * max_edge. PixelSource::TileSynth. No source I/O and no soft-ladder clamp —
   * suitable for 512–kBatchMaxEdge (and higher) when the pyramid exists.
   */
  [[nodiscard]] std::optional<PixelLevel> get_pixels_from_tiles(
      std::string_view uri, int max_edge) const;

  /// Cache-only: inline LQIP (ThumbHash) on the content row — no blob I/O.
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> get_lqip(
      std::string_view uri) const;

  /// Cache-only EXIF/container JPEG preview (kind EmbeddedJpeg). Not LQIP.
  [[nodiscard]] std::optional<EmbeddedPreview> get_embedded_preview(
      std::string_view uri) const;

  /// Free-data only: if LQIP is missing, encode from an already-available soft
  /// or TileSynth overview (≤64 long edge). **Never opens the source.** Returns
  /// get_lqip. No-op when LQIP is already stored (does not upgrade kinds).
  /// See docs/PIXEL_AND_ARCHIVE_POLICY.md §1.1.
  std::optional<std::vector<std::uint8_t>> ensure_lqip(std::string_view uri);

  /// Queue a free-data-only ensure_lqip on a worker. Hosts must not use this to
  /// “generate LQIP”; LQIP is filled opportunistically during tile/soft encode.
  void request_lqip(std::string uri);

  void request_size(std::string uri, SizeCallback cb);

  /// Ensure a soft ladder level exists (probe if needed), then invoke  cb.
  ///
  /// Contract:
  /// -  max_edge is clamped to kMaxSoftLadderEdge (512). Larger values do not
  ///   create full-page 1024/2048 JXL levels — use request_tile or a consumer
  ///   full decode for high resolution.
  /// - If a cached level already **covers** the (clamped) request (~90% of
  ///   long edge in actual pixels), the callback runs immediately.
  /// - A smaller cached level (e.g. 256 when asking for 512) does **not**
  ///   short-circuit; EnsurePixels upgrades the soft ladder.
  /// - Callback may still deliver only the best soft level if encode fails.
  void request_pixels(std::string uri, int max_edge, PixelsCallback cb,
                      int frame_idx = 0);

  /**
   * FastBatch / overview path (PIXEL_PIPELINE Lane A).
   * max_edge is clamped to kBatchMaxEdge (1024). Prefers cache (soft +
   * TileSynth). On miss: shrink-decode source once (JpegShrink), reply with
   * that level; durable soft is still only written ≤ kMaxSoftLadderEdge.
   */
  void request_overview_pixels(std::string uri, int max_edge,
                               PixelsCallback cb);
  /// Full / near-native level (≤ kFullMaxEdge). max_edge 0 → kFullMaxEdge.
  void request_full_pixels(std::string uri, int max_edge, PixelsCallback cb);
  /// Cache-only largest level for uri (optional max_edge cap). DEBUG_OVERLAY stamps.
  [[nodiscard]] std::optional<PixelLevel> get_full_pixels(std::string_view uri,
                                                          int max_edge = 0) const;

  /**
   * Cache-only unified raster lookup (PIXEL_PIPELINE).
   * SoftOnly: soft ladder only (no TileSynth).
   * PreferCache / Overview: TileSynth via get_pixels when tiles exist.
   */
  [[nodiscard]] std::optional<PixelLevel> get_raster(const RasterRequest& req) const;

  /**
   * Unified async raster:
   * SoftOnly → request_pixels (ephemeral soft encode only).
   * PreferCache / Overview → TileSynth hit, else request_tile_pyramid + one-shot
   * overview/soft reply while durable tiles build (Kill Soft Phase D).
   */
  void request_raster(RasterRequest req, PixelsCallback cb);

  /// Cache-only grid tile (Phase 4 / Galapix). See TILES.md.
  [[nodiscard]] bool has_tile(std::string_view uri, int scale, int x,
                              int y) const;
  [[nodiscard]] std::optional<TileBlob> get_tile(std::string_view uri, int scale,
                                                 int x, int y) const;

  /// Cache-only coverage: min/max scale present and native size when known.
  [[nodiscard]] std::optional<TileCoverage> get_tile_coverage(
      std::string_view uri) const;

  /// Ensure tile at (scale,x,y) exists; builds [scale..max] in one pass if missing.
  /// Async: always enqueued (never does blob I/O on the caller thread).
  /// Durable Store hits reply via Executor immediately (no worker queue);
  /// misses are enqueued for encode. Callback always via Executor.
  void request_tile(std::string uri, int scale, int x, int y, TileCallback cb);

  /// One interactive worker job for many cells of the same URI (shared size
  /// probe / shrink ladder). \a cb is invoked once per coordinate.
  struct TileCoord {
    int scale = 0;
    int x = 0;
    int y = 0;
  };
  /// \a on_cell is invoked once per coordinate (index matches \a coords).
  /// Prefer this over a shared TileCallback that must re-match scale/x/y —
  /// missed matches left Galapix JobHandles REQUESTED forever.
  using TileBatchCallback =
      std::function<void(std::size_t index, std::optional<TileBlob> tile)>;
  /// Legacy wrapper over request_tile_cells: Ok → tile, Failed/Unavailable →
  /// nullopt, Cancelled → **no call** (old contract). New hosts should use
  /// request_tile_cells, which never drops a cell.
  void request_tiles(std::string uri, std::vector<TileCoord> coords,
                     TileBatchCallback on_cell);

  /// Interactive viewport cells with an explicit per-cell outcome.
  ///
  /// Contract (normative, see TILES.md "Interactive cell contract"):
  /// - \a on_cell is invoked **exactly once** per index, always via Executor,
  ///   in completion order (streamed: a cell is delivered as soon as it is
  ///   produced, not after the whole batch).
  /// - Cancellation (cancel_tile_cells / cancel_uri / cancel_pending /
  ///   shutdown) delivers TileStatus::Cancelled for every cell that was not
  ///   produced yet. A cell already being computed may still deliver Ok.
  /// - Worker exceptions deliver TileStatus::Failed with the exception text.
  /// - Jobs are **not** dropped by bump_interest_epoch / set_interest; the
  ///   host owns their lifetime via cancel_tile_cells.
  /// - No same-cell supersede: the host must not request a cell that it
  ///   already has outstanding (thumtoo does not merge duplicates).
  using TileResultCallback = std::function<void(std::size_t index, TileResult)>;
  void request_tile_cells(std::string uri, std::vector<TileCoord> coords,
                          TileResultCallback on_cell);

  void invalidate_tile(std::string_view uri, int scale, int x, int y);

  /// Prewarm pyramid [min_scale..max_scale] (max_scale < 0 → until single tile).
  void request_tile_pyramid(std::string uri, int min_scale = 0,
                            int max_scale = -1, TileCallback on_done = {});

  /// Register paths, schedule size probes. Returns how many probe jobs were
  /// enqueued (already-ready locators are skipped). Optional callback is
  /// invoked once per completed probe (same path as request_size).
  ///
  /// Archive paths (zip/cbz/rar/…) are expanded: TOC is refreshed, image
  /// members are registered as `file://…//archive:member` locators and probed
  /// up to kArchiveMaxPrepareTotalUncompressedBytes per archive.
  size_t prepare_paths(const std::vector<std::filesystem::path>& paths,
                       SizeCallback on_each = {});

  /// Cache-only TOC if present.
  [[nodiscard]] std::vector<ArchiveEntryRow> get_archive_entries(
      std::string_view archive_uri) const;

  /// Read TOC from source (libarchive), store under archive_uri, return entries.
  std::vector<ArchiveEntryRow> refresh_archive_toc(
      const std::filesystem::path& archive_path);

  /// Document kinds for durable page-count index (PDF / DjVu / EPUB).
  enum class DocumentKind { Pdf = 0, Djvu = 1, Epub = 2 };

  /// Store-first page count (`media.page_count` via file:// locator). On miss,
  /// opens the document when the file exists, mirrors page_count into the
  /// Store, and returns it. Returns a previously stored count even when the
  /// source file is missing (archive `container_member` TOC parity).
  /// layout is used for Epub live probes (default layout if null).
  [[nodiscard]] std::optional<int> document_page_count(
      const std::filesystem::path& path, DocumentKind kind,
      const EpubLayout* layout = nullptr);

  /// Force re-read from source and replace the durable page_count index row.
  std::optional<int> refresh_document_index(
      const std::filesystem::path& path, DocumentKind kind,
      const EpubLayout* layout = nullptr);

  /// Open the PDF once and write Store region width/height for every page that
  /// still lacks dims. No tiles, no thumbs, no soft — size index only.
  /// Returns the number of pages that received a size (0 if open failed).
  /// Concurrent callers for the same path coalesce (one open).
  int ensure_pdf_page_sizes(const std::filesystem::path& path,
                            PdfBackend backend = PdfBackend::Default);

  /// PDF page count (1-based pages). nullopt if no backend can open the file.
  /// Source-only (no durable index). Prefer document_page_count for hosts.
  [[nodiscard]] static std::optional<int> pdf_page_count(
      const std::filesystem::path& path,
      PdfBackend backend = PdfBackend::Default);

  /// Rasterize one page (1-based) to RGB888; empty rgb on failure.
  struct PdfPageRaster {
    int width = 0;
    int height = 0;
    std::vector<std::uint8_t> rgb;
  };
  [[nodiscard]] static std::optional<PdfPageRaster> pdf_rasterize_page(
      const std::filesystem::path& path, int page_1based, int max_edge,
      PdfBackend backend = PdfBackend::Default);

  /// file:///abs.pdf//page:N (or //mupdf-page:N).
  [[nodiscard]] static std::string pdf_page_uri(
      const std::filesystem::path& path, int page_1based,
      PdfBackend backend = PdfBackend::Default);

  [[nodiscard]] static bool is_pdf_path(const std::filesystem::path& path);

  void drain();

  /**
   * Interest epoch for cancel-on-scroll (PIXEL_PIPELINE §6).
   * New request_* jobs stamp the current epoch. bump_interest_epoch()
   * increments the epoch and drops queued jobs stamped with an older value
   * (callbacks get nullopt / empty so hosts can clear inflight state).
   * In-flight workers may still finish; hosts should ignore stale epochs.
   */
  [[nodiscard]] std::uint64_t interest_epoch() const;

  /// Snapshot of worker queue pressure (PIXEL_PIPELINE host metrics).
  struct QueueStats {
    std::size_t pending = 0;
    int inflight = 0;
    int focus_full_inflight = 0;
    std::uint64_t interest_epoch = 0;
    std::size_t size_probe_queued = 0;
    std::size_t size_probe_running = 0;
  };
  [[nodiscard]] QueueStats queue_stats() const;
  /** Live activity snapshot (size probes in phase 1). */
  [[nodiscard]] ActivitySnapshot activity_snapshot() const;

  /**
   * If the job queue is empty and nothing is in-flight, drop orphaned
   * activity-ledger entries (queued notes whose jobs finished without
   * note_*_finished). Keeps host "Working" badges honest.
   */
  void reconcile_activity_if_idle();
  /// Increment epoch and purge stale queued jobs. Returns the new epoch.
  std::uint64_t bump_interest_epoch();
  /// Drop all queued jobs (any epoch); does not touch in-flight work.
  /// Returns how many jobs were removed from the queue.
  std::size_t cancel_pending();
  /// Drop queued jobs whose uri matches (exact). Returns removed count.
  std::size_t cancel_uri(std::string_view uri);

  /// Cancel interactive cells of one uri (scroll cancel). Does not cancel
  /// FocusFull pyramids or other URIs. Legacy single-cell request_tile jobs
  /// are dropped silently (old contract). request_tile_cells cells — queued
  /// or inside a running batch — deliver TileStatus::Cancelled unless they
  /// were already produced. Returns the number of cells cancelled.
  std::size_t cancel_tile_cells(std::string_view uri,
                                std::span<const TileCoord> cells);

  /**
   * Forget a location: cancel queued work, drop the locator row, and if that
   * content_id has no remaining locators, purge tile/level blobs + metadata.
   * Leaves the source file on disk; next get_size/get_pixels miss (cold).
   * Shared content (multiple locators) only loses this URI until the last one.
   */
  /// Also deletes page_text_layer rows for the URI page (or whole blob).
  [[nodiscard]] PurgeStats purge_uri(std::string_view uri,
                                               bool dry_run = false);
  /**
   * Forget every locator whose outer_path matches @p path (absolute preferred).
   * Also tries file:/// URI for the path. Useful for debug "make this cold".
   */
  [[nodiscard]] PurgeStats purge_path(
      const std::filesystem::path& path, bool dry_run = false);

  /// Forget all locators under a URI prefix (Store::forget_uri_prefix).
  [[nodiscard]] PurgeStats purge_uri_prefix(std::string_view uri_prefix,
                                           bool dry_run = false);


  /**
   * Replace the interest snapshot (PIXEL_PIPELINE §6.1).
   * Bumps the interest epoch (cancels stale queued work), then enqueues
   * background work for the new set:
   * - Near / Speculative: request_overview_pixels at min(edge, kBatchMaxEdge)
   * - Primary: same for now; FocusFull (tiles) is a later phase
   * Callbacks are optional; nullopt is fine for pure scheduling.
   * @return the new interest epoch
   */
  std::uint64_t set_interest(std::vector<InterestItem> items);

  /// Tags attach to content_id (sha256:… preferred). URI resolves via locator.
  [[nodiscard]] std::vector<std::string> get_tags(std::string_view uri) const;

  /// Cache-only text layer (nullopt if not stored).
  [[nodiscard]] std::optional<PageTextLayer> get_page_text_layer(
      std::string_view uri) const;

  /// Extract if missing, always store when content_id is known. Source I/O.
  /// Native structured text only (not OCR).
  std::optional<PageTextLayer> ensure_page_text_layer(std::string_view uri);

  /// Cache-only OCR text layer (dual Store slot). nullopt if never OCR'd.
  [[nodiscard]] std::optional<PageTextLayer> get_ocr_page_text_layer(
      std::string_view uri, std::string_view engine = "tesseract",
      std::string_view model = "default") const;

  /// Run OCR (or return cached OCR layer). User-triggered; does not replace
  /// the native layer. force=true re-runs OCR even when a cached OCR layer exists.
  std::optional<PageTextLayer> ensure_ocr_page_text_layer(
      std::string_view uri, const OcrOptions& opts = {}, bool force = false);

  /// Cache-only outline.
  [[nodiscard]] std::optional<DocumentOutline> get_document_outline(
      std::string_view uri) const;

  /// Extract + cache outline when content_id known.
  std::optional<DocumentOutline> ensure_document_outline(std::string_view uri);

  /// Returns false if uri has no content_id yet.
  bool add_tag(std::string_view uri, std::string_view tag,
               std::string_view source = "user");
  bool remove_tag(std::string_view uri, std::string_view tag);

 private:
  explicit Client(std::unique_ptr<Store> store,
                  Executor executor, unsigned worker_threads);

  /// Load/refresh TOC-ordered image members for FastBatch cursor (source I/O on miss).
  void ensure_archive_cursor(const std::filesystem::path& archive_path);

  /// Plan TOC-ordered extract window; advances cursor after successful extract
  /// when advance=true (worker path).
  [[nodiscard]] std::vector<std::string> plan_and_maybe_advance_cursor(
      const std::filesystem::path& archive_path,
      const std::vector<std::string>& interest_members,
      bool advance_after = false);

  enum class JobKind { ProbeSize, EnsurePixels, EnsureTiles, EnsureLqip };

  /// Shared reply state of one request_tile_cells job. Lives in the queued Job
  /// and, while a worker runs it, in running_cell_batches_ so cancel can reach
  /// cells that have not been produced yet. Defined in client.cpp.
  struct TileCellBatch;
  struct Job;
  /// Deliver the one result for cell \a index (no-op if already replied).
  void reply_tile_cell(const std::shared_ptr<TileCellBatch>& batch,
                       std::size_t index, TileResult result);
  /// Cancel matching pending cells (all when \a cells is empty). Queued:
  /// reply Cancelled now. Running: mark so the worker replies Cancelled.
  /// Returns the number of cells affected.
  std::size_t cancel_cells_in_batch(const std::shared_ptr<TileCellBatch>& batch,
                                    std::span<const TileCoord> cells,
                                    bool running, const char* why);
  /// Worker body for request_tile_cells jobs.
  void run_tile_cell_batch(Job& job);
  /// materialize_tile_cell with a failure classification for TileResult.
  [[nodiscard]] TileResult materialize_tile_result(const std::string& uri,
                                                   int scale, int x, int y,
                                                   bool skip_probe);
  /// Render one PDF page cell from the shared display list; durable JPEG for
  /// scale >= kPdfMinDurableTileScale when \a content_id is known. The
  /// status and reason come straight from the renderer.
  [[nodiscard]] TileResult render_pdf_cell(const std::string& content_id,
                                           const ParsedPdfUri& pdf, int scale,
                                           int x, int y);
  /// Same contract for DjVu pages.
  [[nodiscard]] TileResult render_djvu_cell(const std::string& content_id,
                                            const ParsedDjvuUri& djvu, int scale,
                                            int x, int y);
  /// Same contract for EPUB pages.
  [[nodiscard]] TileResult render_epub_cell(const std::string& content_id,
                                            const ParsedEpubUri& epub, int scale,
                                            int x, int y);
  /// Shared tail: rgb888 live blob + durable JPEG for scale >= the durable floor.
  [[nodiscard]] TileResult finish_document_cell(const std::string& content_id,
                                                std::vector<std::uint8_t> rgb,
                                                int width, int height, int scale,
                                                int x, int y, TileSource source);


  struct Job {
    JobKind kind = JobKind::ProbeSize;
    std::uint64_t epoch = 0;  // interest epoch at enqueue time
    std::uint64_t activity_id = 0;  // ActivityLedger id (0 = none)
    std::string uri;
    int max_edge = 0;
    int frame_idx = 0;
    /// EnsurePixels: true → FastBatch overview (reply ≤ kBatchMaxEdge).
    bool overview = false;
    /// Full-native path: edge_limit up to kFullMaxEdge.
    bool full_native = false;
    int tile_scale = 0;
    int tile_x = 0;
    int tile_y = 0;
    int tile_min_scale = 0;
    int tile_max_scale = -1;  // <0 → until single-tile coverage
    bool tile_pyramid = false;  // true: generate range, no single-tile reply
    /// When true, interactive RGB cells are replied without JPEG/SQLite write
    /// (batch paints the whole view first; durable store can follow later).
    bool skip_durable = false;
    /// Batch already probed size; skip handle_probe_size in children.
    bool skip_probe = false;
    /// Non-empty: interactive multi-cell batch for the same uri.
    std::vector<TileCoord> tile_batch;
    /// request_tile_cells: per-cell exactly-once reply state (see TileCellBatch).
    std::shared_ptr<TileCellBatch> cells;
    /// true → enqueue does not stamp the interest epoch, so bump_interest_epoch
    /// never drops the job (host-owned lifetime via cancel_tile_cells).
    bool epoch_exempt = false;
    SizeCallback size_cb;
    PixelsCallback pixels_cb;
    TileCallback tile_cb;
  };

  void worker_main();
  /// Deliver nullopt/empty callbacks for a job removed from the queue.
  void reply_cancelled_job(Job& job);
  /// \param front true → LIFO (interactive tiles); false → FIFO (bulk).
  void enqueue(Job job, bool front = false);
  /// Enqueue many jobs under one lock, then notify_all once. Critical for
  /// prepare_paths / archive size batches so coalesce sees the full set
  /// instead of racing partial queues (N solid RAR restarts).
  void enqueue_jobs(std::vector<Job> jobs, bool front = false);
  /// Decrement inflight_; notify drain waiters when queue empty and idle.
  void release_inflight_locked();
  /// Write newly encoded tiles into Store bulk (by content_id hash).
  void put_tiles_to_store(const std::string& content_id,
                             const std::vector<TileBlob>& tiles);

  /// Resolve pure/page content_id to Store media+region for tile read fallback.
  struct StoreTileTarget {
    std::int64_t media_id = 0;
    std::int64_t region_id = 0;
  };
  [[nodiscard]] std::optional<StoreTileTarget> store_tile_target_for_content_id(
      std::string_view content_id) const;

  /// ContentMeta from Store locator + media/regions.
  [[nodiscard]] std::optional<ContentMeta> meta_from_store(
      std::string_view uri) const;
  [[nodiscard]] LocatorRow locator_row_from_store(
      const Store::LocatorRow& sl) const;

  /// Ensure a Store blob + locators for an archive container (no full-file hash).
  [[nodiscard]] std::optional<std::int64_t> ensure_store_container_blob(
      const std::filesystem::path& archive_path);

  void handle_probe_size(
      Job& job,
      const std::optional<std::vector<std::uint8_t>>& preextracted = std::nullopt);
  /// Probe into Store. When @p preextracted is set (archive batch extract),
  /// do not call member_bytes again — that was re-reading/re-extracting every
  /// size probe and dominated cold RAR timing (~46s local for 164 members).
  void handle_probe_size_store(
      Job& job,
      const std::optional<std::vector<std::uint8_t>>& preextracted = std::nullopt);
  void handle_ensure_pixels(
      Job& job,
      const std::optional<std::vector<std::uint8_t>>& preextracted = std::nullopt);
  void handle_ensure_pixels_store(Job& job);
  /// Store hit or encode one grid cell (interactive). Writes durable JPEG when
  /// appropriate. Does not post callbacks.
  [[nodiscard]] std::optional<TileBlob> materialize_tile_cell(
      const std::string& uri, int scale, int x, int y, bool skip_probe);
  void handle_ensure_tiles_store(Job& job);
  void handle_ensure_tiles(
      Job& job,
      const std::optional<std::vector<std::uint8_t>>& preextracted = std::nullopt);
  void handle_ensure_lqip(Job& job);
  void store_tiles(const std::string& content_id,
                   const std::vector<TileBlob>& tiles);
  /// Drop JpegShrink (Q1) soft levels once a durable tile pyramid exists.
  void invalidate_q1_levels(const std::string& content_id);

  static constexpr std::size_t kExtractCacheMaxBytes = 512ull * 1024ull * 1024ull;
  /// Same budget shared conceptually; HTTP bodies use a separate map.
  static constexpr std::size_t kHttpCacheMaxBytes = 512ull * 1024ull * 1024ull;
  [[nodiscard]] static std::string extract_cache_key(
      const std::filesystem::path& archive, std::string_view member);
  void extract_cache_put(const std::filesystem::path& archive,
                         std::string_view member,
                         std::vector<std::uint8_t> bytes);
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> extract_cache_get(
      const std::filesystem::path& archive, std::string_view member) const;
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> member_bytes(
      const std::filesystem::path& archive, std::string_view member,
      const std::optional<std::vector<std::uint8_t>>& preextracted = std::nullopt);

  /// Disk extract staging (sequential archives). Survives process RAM LRU so
  /// solid RAR is not re-decompressed for size → soft → tiles. Under
  /// cache_root/extract_staging when durable; else $TMPDIR/thumtoo-extract-*.
  [[nodiscard]] std::filesystem::path extract_staging_root() const;
  [[nodiscard]] std::filesystem::path extract_staging_path(
      const std::filesystem::path& archive, std::string_view member) const;
  void extract_staging_put(const std::filesystem::path& archive,
                           std::string_view member,
                           const std::vector<std::uint8_t>& bytes);
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> extract_staging_get(
      const std::filesystem::path& archive, std::string_view member) const;
  [[nodiscard]] bool extract_staging_has(
      const std::filesystem::path& archive, std::string_view member) const;

  /// Optional local mirror (THUMTOO_MIRROR_ARCHIVES=1). Default no-op.
  [[nodiscard]] std::filesystem::path ensure_local_archive(
      const std::filesystem::path& archive);

  /// GET with in-process cache (session only; not durable across runs).
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> fetch_http_cached(
      std::string_view url);

  std::unique_ptr<Store> store_;
  Executor executor_;

  mutable std::mutex mu_;  // also locked from const interest_epoch()
  std::condition_variable cv_;
  std::deque<Job> queue_;
  bool stop_ = false;
  int inflight_ = 0;
  /// FocusFull tile-pyramid jobs currently running (worker claimed).
  int focus_full_inflight_ = 0;
  std::uint64_t interest_epoch_ = 1;
  /// request_tile_cells batches currently claimed by a worker (guarded by mu_).
  std::vector<std::shared_ptr<TileCellBatch>> running_cell_batches_;
  std::vector<std::thread> workers_;

  mutable std::mutex archive_cursor_mu_;
  std::unordered_map<std::string, ArchiveCursor> archive_cursors_;

  /// Serialize solid/sequential disk extracts so concurrent member_bytes calls
  /// do not each restart a full RAR/tar walk (second caller hits extract cache).
  mutable std::mutex sequential_extract_mu_;

  mutable std::mutex extract_cache_mu_;

  /// LRU: front = most recently used. Values hold bytes + list iterator.
  /// Cache maps are mutable so const get() can touch the LRU order.
  struct ExtractCacheEntry {
    std::vector<std::uint8_t> bytes;
    std::list<std::string>::iterator lru_it;
  };
  mutable std::list<std::string> extract_cache_lru_;
  mutable std::unordered_map<std::string, ExtractCacheEntry> extract_cache_;
  mutable std::size_t extract_cache_bytes_ = 0;

  mutable std::mutex http_cache_mu_;
  std::unordered_map<std::string, std::vector<std::uint8_t>> http_cache_;
  std::size_t http_cache_bytes_ = 0;

  /// Coalesce ensure_pdf_page_sizes for the same path (Client-owned, not process
  /// static). Key = lexically_normal path string.
  mutable std::mutex ensure_pdf_mu_;
  mutable std::unordered_map<std::string, std::shared_ptr<std::mutex>>
      ensure_pdf_path_mu_;
};

}  // namespace thumtoo
