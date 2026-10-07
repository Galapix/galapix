// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/status.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace thumtoo {

struct Size {
  int width = 0;
  int height = 0;
};

/// Origin of an embedded container preview (not content-derived tiles/LQIP).
enum class EmbeddedOrigin : int {
  Unknown = 0,
  ExifJpeg = 1,     ///< EXIF IFD1 / APP1 JPEG thumbnail
  PdfPageThumb = 2, ///< PDF page /Thumb stream (when present)
  Other = 3,
};

/// Cheap first-paint underlay from container metadata (EXIF, PDF /Thumb, …).
/// Never written into the tile pyramid. Hosts must tag provenance and still
/// schedule tiles / content-derived work.
struct EmbeddedPreview {
  std::vector<std::uint8_t> bytes;  ///< typically JPEG
  int width = 0;
  int height = 0;
  EmbeddedOrigin origin = EmbeddedOrigin::Unknown;
};

/// Result of request_size / ProbeSize — native size plus cache-only underlays.
struct SizeReply {
  std::optional<Size> size;
  /// ThumbHash / Handsum blob from the content row; empty if not stored yet.
  /// Never Embedded JPEG (see embedded).
  std::optional<std::vector<std::uint8_t>> lqip;
  /// EXIF / container preview when available — distinct from tiles and LQIP.
  std::optional<EmbeddedPreview> embedded;
};

struct ContentMeta {
  std::string content_id;
  std::optional<Size> size;
  std::optional<std::int64_t> duration_ms;
  std::optional<int> still_count;
  ContentStatus status = ContentStatus::Pending;
  std::optional<std::string> error_code;
  std::optional<std::string> format;
};

/// How a soft ladder level was produced (stored on levels + PixelLevel).
/// Hosts must not treat Embedded as a completed soft ladder of the requested edge.
enum class PixelSource : int {
  Unknown = 0,      ///< Legacy row / not tagged
  JpegShrink = 1,   ///< vips_thumbnail / shrink-on-decode from full source
  Embedded = 2,     ///< EXIF or other embedded JPEG thumbnail (may be tiny / off-aspect)
  Full = 3,         ///< Full-resolution or near-native extract stored as a level
  TileSynth = 4,    ///< Full-frame raster reconstructed from grid tiles
};

inline constexpr std::string_view to_string(PixelSource s) {
  switch (s) {
    case PixelSource::Unknown:
      return "unknown";
    case PixelSource::JpegShrink:
      return "jpeg_shrink";
    case PixelSource::Embedded:
      return "embedded";
    case PixelSource::Full:
      return "full";
    case PixelSource::TileSynth:
      return "tile_synth";
  }
  return "unknown";
}

/// Unified raster request policy (PIXEL_PIPELINE request_raster).
enum class RasterPolicy : int {
  /// Soft durable only (≤ kMaxSoftLadderEdge); never FastScale or tiles.
  SoftOnly = 0,
  /// Soft, else TileSynth, else overview FastScale ≤ kBatchMaxEdge.
  PreferCache = 1,
  /// Soft, TileSynth, overview FastScale; no full native dump.
  Overview = 2,
  /// Native-ish full level via request_full_pixels (≤ kFullMaxEdge).
  Full = 3,
};

/// Parameters for Client::get_raster / request_raster.
struct RasterRequest {
  std::string uri;
  int max_edge = 0;  ///< 0 → policy default (soft max or batch max)
  int frame_idx = 0;
  RasterPolicy policy = RasterPolicy::PreferCache;
};

/// Role of a locator in a host interest snapshot (PIXEL_PIPELINE §6.1).
enum class InterestRole : int {
  Speculative = 0,  ///< Idle / overscan — lowest priority
  Near = 1,         ///< Visible or near-visible (FastBatch window)
  Primary = 2,      ///< Focused image(s) — FocusFull path later
};

/// One interest entry for set_interest().
struct InterestItem {
  std::string uri;
  int target_long_edge = 0;  ///< 0 → kBatchMaxEdge for Near/Primary overview
  InterestRole role = InterestRole::Near;
};

/// Encoded ladder level from cache (JPEG-XL bytes by default).
struct PixelLevel {
  int max_edge = 0;
  int frame_idx = 0;
  int width = 0;
  int height = 0;
  std::string codec;  // "jxl"
  std::vector<std::uint8_t> bytes;
  PixelSource source = PixelSource::Unknown;
};

/// How the tile pixels were produced (stored in index for cache policy).
enum class TileSource : int {
  Full = 0,         ///< Full-resolution decode (or non-JPEG path)
  JpegShrink = 1,   ///< libjpeg/vips DCT shrink (scale > 0)
  Embedded = 2,     ///< EXIF/embedded thumbnail
  PdfRegion = 3,    ///< PDF region raster
  DjvuRegion = 4,   ///< ddjvu region raster
};

/// Encoded grid tile (Phase 4 / Galapix-compatible). See TILES.md.
struct TileBlob {
  int scale = 0;
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
  std::string codec;  // "jpeg" by default
  std::vector<std::uint8_t> bytes;
  TileSource source = TileSource::Full;
};

/// Outcome of one interactive tile cell (Client::request_tile_cells).
/// Every requested cell gets exactly one result — never silence.
enum class TileStatus : std::uint8_t {
  Ok = 0,           ///< TileResult::tile holds the pixels
  Cancelled = 1,    ///< Host cancel / shutdown before the cell was produced; retry is fine
  Failed = 2,       ///< Could not produce the cell (I/O, decode, internal); see error
  Unavailable = 3,  ///< Cell can never exist for this content (outside grid,
                    ///< unsupported scale); retrying is pointless
};

[[nodiscard]] constexpr const char* tile_status_name(TileStatus s) noexcept {
  switch (s) {
    case TileStatus::Ok: return "ok";
    case TileStatus::Cancelled: return "cancelled";
    case TileStatus::Failed: return "failed";
    case TileStatus::Unavailable: return "unavailable";
  }
  return "?";
}

struct TileResult {
  TileStatus status = TileStatus::Failed;
  std::optional<TileBlob> tile;  ///< Set iff status == Ok
  std::string error;             ///< Human-readable reason when status != Ok
};

struct TileCoverage {
  int min_scale = 0;
  int max_scale = 0;
  Size size;
};

}  // namespace thumtoo
