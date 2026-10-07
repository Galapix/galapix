// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace thumtoo {

/// Historical schema-4 ladder version (on-disk migrate detection only).
/// Legacy schema-4 ladder version constants (historical; not used by Client).
inline constexpr int kSchemaVersion = 4;

/// Redesign Store (docs/DATABASE.md): index/bulk/user. Values ≥ 100 so open()
/// Redesign Store index starts at 100 so tools can refuse schema-4 indexes.
/// 100 = redesign cutover. 101 = directory POSIX columns + user uuid keys
/// (additive; see docs/DIRTOO_BACKBONE.md). bulk unchanged layout at 100.
inline constexpr int kStoreIndexSchemaVersion = 101;
inline constexpr int kStoreBulkSchemaVersion = 100;
inline constexpr int kStoreUserSchemaVersion = 101;

/// directory_entry.file_type (0 = unknown / legacy row).
inline constexpr int kFsFileTypeUnknown = 0;
inline constexpr int kFsFileTypeReg = 1;
inline constexpr int kFsFileTypeDir = 2;
inline constexpr int kFsFileTypeLnk = 3;
inline constexpr int kFsFileTypeChr = 4;
inline constexpr int kFsFileTypeBlk = 5;
inline constexpr int kFsFileTypeFifo = 6;
inline constexpr int kFsFileTypeSock = 7;

/// Policy long-edge steps (pixels). Soft whole-image replies clamp to
/// kMaxSoftLadderEdge (ephemeral; not Store-durable). 2048+ steps are for
/// Full / display (request_full_pixels), not soft. See PIXEL_AND_ARCHIVE_POLICY.md.
inline constexpr std::array<int, 7> kLadderEdges = {
    128, 256, 512, 1024, 2048, 4096, 8192};
/// Maximum long edge for ephemeral soft / request_pixels (not Store-durable).
/// Requests above this are clamped. Full uses kFullMaxEdge via
/// request_full_pixels. See docs/PIXEL_AND_ARCHIVE_POLICY.md and TILES.md.
inline constexpr int kMaxSoftLadderEdge = 512;
/// Cap for FastBatch / shrink overview rasters (ephemeral / TileSynth band).
inline constexpr int kBatchMaxEdge = 1024;
/// Cap for request_full_pixels / Full policy (native-ish display level).
inline constexpr int kFullMaxEdge = 8192;
/// Max archive members planned per FastBatch extract window (PIXEL_PIPELINE §5.2).
inline constexpr int kBatchWindowMembers = 32;
/// Max concurrent FocusFull tile-pyramid jobs in the queue (PIXEL_PIPELINE §8).
inline constexpr int kFocusFullMaxConcurrent = 1;
/// Speculative interest only enqueues when pending queue length is below this.
inline constexpr std::size_t kSpeculativeEnqueueWhenQueueBelow = 8;

/// Ladder codec is JPEG-XL via libvips (required dependency; see flake.nix).
inline constexpr int kDefaultJxlQuality = 80;
inline constexpr char kDefaultCodec[] = "jxl";

/// Video temporal stills (plus poster as frame_idx 0).
inline constexpr int kDefaultVideoStillCount = 16;

inline constexpr std::string_view kContentIdSha256Prefix = "sha256:";
inline constexpr std::string_view kContentIdProvisionalPrefix = "prov:";

/// Archive security defaults (DESIGN §6b).
inline constexpr std::uint64_t kArchiveMaxMemberUncompressedBytes =
    512ull * 1024ull * 1024ull;
inline constexpr int kArchiveMaxCompressionRatio = 100;
inline constexpr std::uint64_t kArchiveMaxPrepareTotalUncompressedBytes =
    2ull * 1024ull * 1024ull * 1024ull;

inline constexpr char kSchemaMetaVersionKey[] = "schema_version";
inline constexpr char kSchemaMetaLadderEdgesKey[] = "ladder_edges";
inline constexpr char kSchemaMetaJxlQualityKey[] = "jxl_quality";
// Legacy name still accepted when reading old caches.
inline constexpr char kSchemaMetaWebpQualityKey[] = "webp_quality";

/// PDF page layout / live tile rasterization: media box scaled to this DPI
/// (144 = 2× 72). Scale 0 matches get_size; negative scales are denser
/// (dpi = kPdfLayoutDpi * 2^{-scale}). Do not change without a cache migration.
inline constexpr int kPdfLayoutDpi = 144;

/// Finest scale written to the durable tile DB for PDF pages. Finer scales
/// (e.g. -3, -4) may still be generated live and returned to the client, but
/// are not stored — avoids filling the cache with 1k–9k dpi cells.
inline constexpr int kPdfMinDurableTileScale = -2;  // 144 * 4 = 576 dpi

/// Raster-only PDF pages (no visible vector content or text) stop refining
/// once the rendered dpi reaches the sharpest image's native dpi divided by
/// this tolerance: a 300 dpi scan caps at scale -1 (288 dpi) instead of
/// paying 4x the pixels at -2 for 4% more resolution. Reported in
/// PdfPageProfile::summary.
inline constexpr double kPdfNativeDpiTolerance = 1.25;

/// A first painted fill covering at least this fraction of the page is a
/// paper background (common in scan PDFs), not vector detail. Reported in
/// PdfPageProfile::background_fill_ignored.
inline constexpr double kPdfBackgroundFillCoverage = 0.98;

/// Embedded images whose decoded size (at the requested subsample factor)
/// fits this budget are decoded whole once and kept in the shared MuPDF
/// store, so every tile cell reuses the decode. Larger images fall back to
/// MuPDF's per-cell subarea decode. Counted in PdfDecodeStats.
inline constexpr std::size_t kPdfFullImageDecodeBudget = 128u << 20;  // 128 MiB

/// Shared MuPDF store (decoded images, glyphs) for all worker threads.
inline constexpr std::size_t kPdfStoreBytes = 512u << 20;  // 512 MiB

/// Open documents / cached pages (display list + profile) per document.
inline constexpr int kPdfDocumentCacheSize = 8;
inline constexpr int kPdfPageCacheSize = 32;

/// EPUB default virtual page size in **pixels** at kEpubLayoutDpi and base font
/// size in points for fz_layout_document. Changing these invalidates default
/// expand URIs and cached tiles for that profile.
/// (Legacy used points; URI now carries pixels so image-viewer math stays simple.)
/// ~6.25×9.4 in at 144 dpi — closer to a trade page than a large tablet sheet.
inline constexpr int kEpubDefaultPageWidthPx = 900;
inline constexpr int kEpubDefaultPageHeightPx = 1350;
/// Body text size; also forced via user CSS so document styles cannot ignore it.
inline constexpr int kEpubDefaultFontSizePt = 15;
/// Default line-height ×100 (140 → 1.4).
inline constexpr int kEpubDefaultLineHeightPercent = 140;
/// Layout pixel density for EPUB pages (same convention as PDF layout DPI).
inline constexpr int kEpubLayoutDpi = 144;

/// Grid tiles (Phase 4 / Galapix-compatible). See TILES.md.
/// Exclusive ≤256×256 cells; grid step is kTileSize. No edge overlap.
inline constexpr int kTileSize = 256;

/// Exclusive pixel crop for tile (x,y) on a level of size (sw,sh).
inline void tile_cell_pixel_rect(int sw, int sh, int x, int y,
                                 int* left, int* top, int* tw, int* th) noexcept {
  if (!left || !top || !tw || !th) {
    return;
  }
  *left = x * kTileSize;
  *top = y * kTileSize;
  if (*left >= sw || *top >= sh || x < 0 || y < 0) {
    *tw = 0;
    *th = 0;
    return;
  }
  const int max_w = sw - *left;
  const int max_h = sh - *top;
  *tw = (kTileSize < max_w) ? kTileSize : max_w;
  *th = (kTileSize < max_h) ? kTileSize : max_h;
}

/// Dimension at pyramid @p scale matching successive integer factor-2 shrink
/// (Galapix / vips_shrink 2.0: floor-half each step). Not ceil(n / 2^scale),
/// which drifts by ~1px at coarse scales vs the encode path.
[[nodiscard]] inline int dim_at_tile_scale(int n, int scale) noexcept {
  if (n <= 0) {
    return 0;
  }
  if (scale <= 0) {
    return n;
  }
  for (int i = 0; i < scale; ++i) {
    n /= 2;
    if (n <= 0) {
      return 0;
    }
  }
  return n;
}
inline constexpr int kDefaultTileQuality = 80;
/// PDF live/durable cells: text rings badly at Q=80 when zoomed; use higher.
inline constexpr int kPdfTileQuality = 95;
inline constexpr char kDefaultTileCodec[] = "jpeg";
/// Uncompressed RGB888 payload in TileBlob::bytes (live PDF cells).
inline constexpr char kTileCodecRgb888[] = "rgb888";

/// Durable HTTP body cache TTL (0 = never expire by age).
inline constexpr std::int64_t kHttpCacheTtlSeconds = 7LL * 24 * 3600;
/// Refuse tile encode when width*height exceeds this (memory guard).
inline constexpr std::int64_t kTileMaxSourcePixels = 100000000LL;  // 100 MP

}  // namespace thumtoo
