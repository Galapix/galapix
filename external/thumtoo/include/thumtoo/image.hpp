// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/constants.hpp"
#include "thumtoo/types.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace thumtoo {

struct ProbeResult {
  Size size;
  std::string format;
};

struct LevelBlob {
  int max_edge = 0;
  int frame_idx = 0;
  int width = 0;
  int height = 0;
  std::string codec;  // "jxl"
  int quality = 0;
  std::string relative_path;
  std::vector<std::uint8_t> bytes;
  PixelSource source = PixelSource::JpegShrink;
};

void image_library_init();

/// Build ThumbHash bytes from RGB888 (downscales to ≤32 edge internally).
[[nodiscard]] std::vector<std::uint8_t> lqip_thumbhash_from_rgb888(
    const std::uint8_t* rgb, int width, int height);

/// Load file, thumbnail to ≤32 edge, encode ThumbHash (empty on failure).
[[nodiscard]] std::vector<std::uint8_t> lqip_thumbhash_from_file(
    const std::filesystem::path& path);

/// Decode buffer, thumbnail to ≤32 edge, encode ThumbHash.
[[nodiscard]] std::vector<std::uint8_t> lqip_thumbhash_from_buffer(
    const std::uint8_t* data, std::size_t size);


/// Read a regular file up to max_bytes (rejects larger). Empty on error.
[[nodiscard]] std::optional<std::vector<std::uint8_t>> read_file_bytes(
    const std::filesystem::path& path, std::uint64_t max_bytes);

[[nodiscard]] std::optional<ProbeResult> probe_image_file(
    const std::filesystem::path& path);

[[nodiscard]] std::optional<ProbeResult> probe_image_buffer(
    const std::uint8_t* data, std::size_t size, std::string_view hint_format = {});

/// EXIF IFD1 JPEG thumbnail when present (no full decode of the primary image).
/// Dimensions from the embedded JPEG header when decodable; bytes are JPEG.
[[nodiscard]] std::optional<EmbeddedPreview> try_exif_embedded_preview_file(
    const std::filesystem::path& path);
[[nodiscard]] std::optional<EmbeddedPreview> try_exif_embedded_preview_buffer(
    const std::uint8_t* data, std::size_t size);

/// Encode one durable preview level: the largest kLadderEdges entry that is
/// ≤ max_edge_limit and ≤ the source long edge (single vips_thumbnail pass).
/// max_edge_limit ≤ 0 means "largest ladder edge that fits the source".
/// Does not build a full multi-edge ladder — callers that need a smaller
/// proxy can downscale this level, or request_pixels with a smaller max_edge.
[[nodiscard]] std::vector<LevelBlob> build_ladder(
    const std::filesystem::path& path, const std::string& content_id,
    int jxl_quality, int max_edge_limit = 0);

[[nodiscard]] std::vector<LevelBlob> build_ladder_buffer(
    const std::uint8_t* data, std::size_t size, const std::string& content_id,
    int jxl_quality, int max_edge_limit = 0);

/// Build JXL preview from contiguous RGB888 pixels (no alpha). Same single-edge
/// policy as build_ladder.
[[nodiscard]] std::vector<LevelBlob> build_ladder_rgb(
    const std::uint8_t* rgb, int width, int height, const std::string& content_id,
    int jxl_quality, int max_edge_limit = 0);

/// Encode one JXL level at an exact long-edge target (clamped to source).
/// Used for //pdfimage native storage keyed by source long edge.
[[nodiscard]] std::optional<LevelBlob> build_level_rgb_at_edge(
    const std::uint8_t* rgb, int width, int height, const std::string& content_id,
    int jxl_quality, int target_edge);

/// Downscale an already-cached JXL preview to a smaller policy edge (no source I/O).
[[nodiscard]] std::optional<LevelBlob> downscale_preview_jxl(
    const std::uint8_t* jxl_data, std::size_t jxl_size,
    const std::string& content_id, int target_edge, int jxl_quality);

[[nodiscard]] std::string sha256_file_hex(const std::filesystem::path& path);
[[nodiscard]] std::string sha256_bytes_hex(const std::uint8_t* data,
                                           std::size_t size);

/// Galapix-style 256² JPEG tile pyramid. scale 0 = full res; higher = coarser.
/// Generates [min_scale .. max_scale] inclusive. If max_scale < 0, goes until
/// the image fits in a single tile.
[[nodiscard]] std::vector<TileBlob> build_tile_pyramid(
    const std::filesystem::path& path, int min_scale = 0, int max_scale = -1,
    int jpeg_quality = kDefaultTileQuality);

[[nodiscard]] std::vector<TileBlob> build_tile_pyramid_buffer(
    const std::uint8_t* data, std::size_t size, int min_scale = 0,
    int max_scale = -1, int jpeg_quality = kDefaultTileQuality);

/// Interactive path: encode a single (scale, x, y) cell only (not the whole
/// scale grid). Same shrink/crop conventions as the pyramid builder.
[[nodiscard]] std::optional<TileBlob> build_tile_cell(
    const std::filesystem::path& path, int scale, int x, int y,
    int jpeg_quality = kDefaultTileQuality);

[[nodiscard]] std::optional<TileBlob> build_tile_cell_buffer(
    const std::uint8_t* data, std::size_t size, int scale, int x, int y,
    int jpeg_quality = kDefaultTileQuality,
    std::string_view decode_cache_key = {});

/// Same as build_tile_cell_buffer but from contiguous RGB888 (no alpha).
/// Used for PDF page rasters and other in-memory RGB sources.
[[nodiscard]] std::optional<TileBlob> build_tile_cell_rgb(
    const std::uint8_t* rgb, int width, int height, int scale, int x, int y,
    int jpeg_quality = kDefaultTileQuality);

/// Encode an already-cropped RGB888 buffer as a TileBlob (any scale, including negative).
/// If @p blob is durable JPEG (or other encoded), decode to rgb888 for host
/// paint. Already-rgb888 / empty pass through. Runs on the caller thread
/// (worker); does not touch Store.
[[nodiscard]] std::optional<TileBlob> decode_tile_blob_to_rgb888(TileBlob blob);

[[nodiscard]] std::optional<TileBlob> encode_tile_cell_rgb(
    const std::uint8_t* rgb, int width, int height, int scale, int x, int y,
    int jpeg_quality = kDefaultTileQuality);

[[nodiscard]] std::vector<TileBlob> build_tile_pyramid_rgb(
    const std::uint8_t* rgb, int width, int height, int min_scale = 0,
    int max_scale = -1, int jpeg_quality = kDefaultTileQuality);

}  // namespace thumtoo
