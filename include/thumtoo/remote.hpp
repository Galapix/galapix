// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

/// thumtoo-serve wire protocol (docs/REMOTE.md), shared by the server tool
/// and Client::open_remote. Read-only HTTP; JSON for metadata, raw bytes for
/// tiles.

#include "thumtoo/types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace thumtoo {

inline constexpr int kRemoteApiVersion = 1;

/// Token cookie set by the server after a valid ?token= request.
inline constexpr char kRemoteTokenCookie[] = "thumtoo_token";

/// True when built with libcurl + nlohmann_json (Client::open_remote works,
/// the JSON helpers below are real).
[[nodiscard]] bool remote_available() noexcept;

/// http:// or https:// — a thumtoo-serve base URL rather than a cache dir.
[[nodiscard]] bool is_remote_cache_url(std::string_view s) noexcept;

/// One location as thumtoo-serve describes it (`//info`, `/locations` rows).
struct RemoteInfo {
  std::string uri;
  std::optional<std::string> outer_path;
  std::optional<std::string> member_path;
  std::optional<std::int64_t> file_size;
  std::optional<std::int64_t> mtime_ns;
  std::optional<ContentMeta> meta;
  std::optional<TileCoverage> coverage;
  std::optional<std::vector<std::uint8_t>> lqip;
};

/// One `/locations` page; `next` is the `after` cursor of the next page.
struct RemoteLocations {
  std::vector<RemoteInfo> items;
  std::optional<std::string> next;
};

/// Request path below the base URL:
///   /<location>//info   /<location>//tiles   /<location>//tile:S,X,Y
///   /blob/<content_id>//tile:S,X,Y   (immutable, by content identity)
struct RemoteRoute {
  enum class Kind { Info, Tiles, Tile };
  Kind kind = Kind::Info;
  std::string location;  ///< Location URI, or content_id when by_content_id
  bool by_content_id = false;
  int scale = 0;
  int x = 0;
  int y = 0;
};

/// Parse a percent-decoded request path ("/file:///a.jpg//tile:0,1,2").
[[nodiscard]] std::optional<RemoteRoute> parse_remote_route(
    std::string_view path);

/// Percent-encoded request path for a route (leading '/'). Location bytes
/// outside the unreserved set and "/:,;=@!$&'()*+" are %XX-escaped.
[[nodiscard]] std::string format_remote_route(const RemoteRoute& route);

/// RFC 3986 query-component escape (for ?prefix= / ?like= / ?after=).
[[nodiscard]] std::string remote_query_escape(std::string_view s);

/// MIME type for a TileBlob codec ("jpeg" → "image/jpeg").
[[nodiscard]] const char* tile_codec_mime(std::string_view codec) noexcept;
/// Inverse of tile_codec_mime; "jpeg" for unknown types.
[[nodiscard]] std::string tile_codec_from_mime(std::string_view mime);

[[nodiscard]] std::string base64_encode(const std::vector<std::uint8_t>& data);
[[nodiscard]] std::optional<std::vector<std::uint8_t>> base64_decode(
    std::string_view text);

/// JSON (de)serialization. *_from_json return nullopt on malformed input.
[[nodiscard]] std::string remote_info_to_json(const RemoteInfo& info);
[[nodiscard]] std::optional<RemoteInfo> remote_info_from_json(
    std::string_view json);
[[nodiscard]] std::string remote_locations_to_json(const RemoteLocations& page);
[[nodiscard]] std::optional<RemoteLocations> remote_locations_from_json(
    std::string_view json);

}  // namespace thumtoo
