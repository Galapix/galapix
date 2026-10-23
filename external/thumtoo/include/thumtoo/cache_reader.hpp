// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/store.hpp"
#include "thumtoo/types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace thumtoo {

/// Composite content_id: "sha256:<64 hex>" optionally followed by
/// ":page:N", ":page:N:epub:<layout>" or ":pdfimage:N".
struct ContentIdParts {
  std::vector<std::uint8_t> sha256;  ///< Raw 32-byte digest
  std::optional<int> page_1based;
  std::optional<std::string> epub_layout_key;  // format_epub_layout_params
  std::optional<int> pdfimage_1based;
  /// Region key for Page kind (PDF/DjVu: "N"; EPUB: "N|layout").
  std::string page_region_key;
};

/// nullopt for anything that is not a well-formed composite sha256 id.
[[nodiscard]] std::optional<ContentIdParts> parse_content_id(
    std::string_view content_id);

/// Cache-only reads over a Store: location URI → ContentMeta, tile coverage
/// and tile bytes. Never opens source files and never writes, so it works on
/// Store::open_readonly. Client's get_* paths and thumtoo-serve share it.
///
/// Holds a non-owning Store pointer; same threading rules as the Store.
class CacheReader {
 public:
  explicit CacheReader(const Store& store) : store_(&store) {}

  /// Store media + region that hold the tiles of one content_id.
  struct TileTarget {
    std::int64_t media_id = 0;
    std::int64_t region_id = 0;
  };

  /// ContentMeta from locator + media/regions (size, format, status).
  [[nodiscard]] std::optional<ContentMeta> meta(std::string_view uri) const;

  [[nodiscard]] std::optional<TileTarget> tile_target(
      std::string_view content_id) const;

  /// One stored tile; nullopt on miss. Codec from the tile row.
  [[nodiscard]] std::optional<TileBlob> tile(const TileTarget& target,
                                             int scale, int x, int y) const;
  [[nodiscard]] std::optional<TileBlob> tile(std::string_view uri, int scale,
                                             int x, int y) const;

  /// Stored scale range, or the theoretical range from the native size when
  /// no tiles are stored yet. nullopt when neither is known.
  [[nodiscard]] std::optional<TileCoverage> tile_coverage(
      std::string_view uri) const;

  /// Every stored tile row of the location (pyramids are often sparse).
  [[nodiscard]] std::vector<Store::TileRow> list_tiles(
      std::string_view uri) const;

  /// blob_lqip row key of a location (documents: blob of the page or file).
  struct LqipKey {
    std::int64_t blob_id = 0;
    int page_1based = 0;  // 0 = whole blob
  };
  [[nodiscard]] std::optional<LqipKey> lqip_key(std::string_view uri) const;

  /// ThumbHash / Handsum placeholder; never an embedded JPEG preview.
  [[nodiscard]] std::optional<std::vector<std::uint8_t>> lqip(
      std::string_view uri) const;

  /// Known location URIs (see Store::list_locator_uris).
  [[nodiscard]] std::vector<std::string> list_locations(
      std::string_view prefix, std::string_view after = {},
      int limit = 1000) const;

 private:
  const Store* store_;
};

/// Codec name used in TileBlob::codec ("jpeg", "jxl", "png").
[[nodiscard]] const char* tile_codec_name(CodecId codec) noexcept;

}  // namespace thumtoo
