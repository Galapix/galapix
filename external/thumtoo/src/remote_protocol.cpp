// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/remote.hpp"

#include "thumtoo/status.hpp"

#include <array>
#include <charconv>

#if defined(THUMTOO_HAVE_REMOTE)
#include <nlohmann/json.hpp>
#endif

namespace thumtoo {
namespace {

constexpr std::string_view kInfoSuffix = "//info";
constexpr std::string_view kTilesSuffix = "//tiles";
constexpr std::string_view kTileSuffix = "//tile:";
constexpr std::string_view kBlobPrefix = "blob/";

bool parse_int(std::string_view s, int* out) {
  if (s.empty()) return false;
  const auto* end = s.data() + s.size();
  auto [p, ec] = std::from_chars(s.data(), end, *out);
  return ec == std::errc() && p == end;
}

bool is_unreserved(unsigned char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
         (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_' ||
         c == '~';
}

std::string percent_escape(std::string_view s, std::string_view keep) {
  static constexpr char kHex[] = "0123456789ABCDEF";
  std::string out;
  out.reserve(s.size());
  for (char ch : s) {
    const auto c = static_cast<unsigned char>(ch);
    if (is_unreserved(c) || keep.find(ch) != std::string_view::npos) {
      out.push_back(ch);
    } else {
      out.push_back('%');
      out.push_back(kHex[c >> 4]);
      out.push_back(kHex[c & 0xf]);
    }
  }
  return out;
}

constexpr char kB64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

}  // namespace

bool remote_available() noexcept {
#if defined(THUMTOO_HAVE_REMOTE)
  return true;
#else
  return false;
#endif
}

bool is_remote_cache_url(std::string_view s) noexcept {
  return s.starts_with("http://") || s.starts_with("https://");
}

std::optional<RemoteRoute> parse_remote_route(std::string_view path) {
  if (!path.starts_with('/')) return std::nullopt;
  path.remove_prefix(1);
  RemoteRoute r;
  if (path.ends_with(kInfoSuffix)) {
    r.kind = RemoteRoute::Kind::Info;
    path.remove_suffix(kInfoSuffix.size());
  } else if (path.ends_with(kTilesSuffix)) {
    r.kind = RemoteRoute::Kind::Tiles;
    path.remove_suffix(kTilesSuffix.size());
  } else {
    const auto pos = path.rfind(kTileSuffix);
    if (pos == std::string_view::npos) return std::nullopt;
    const std::string_view coords = path.substr(pos + kTileSuffix.size());
    const auto c1 = coords.find(',');
    const auto c2 = c1 == std::string_view::npos ? c1 : coords.find(',', c1 + 1);
    if (c2 == std::string_view::npos ||
        !parse_int(coords.substr(0, c1), &r.scale) ||
        !parse_int(coords.substr(c1 + 1, c2 - c1 - 1), &r.x) ||
        !parse_int(coords.substr(c2 + 1), &r.y)) {
      return std::nullopt;
    }
    r.kind = RemoteRoute::Kind::Tile;
    path = path.substr(0, pos);
  }
  if (path.starts_with(kBlobPrefix)) {
    r.by_content_id = true;
    path.remove_prefix(kBlobPrefix.size());
  }
  if (path.empty()) return std::nullopt;
  r.location = std::string(path);
  return r;
}

std::string format_remote_route(const RemoteRoute& route) {
  std::string out = "/";
  if (route.by_content_id) out += kBlobPrefix;
  out += percent_escape(route.location, "/:,;=@!$&'()*+");
  switch (route.kind) {
    case RemoteRoute::Kind::Info:
      out += kInfoSuffix;
      break;
    case RemoteRoute::Kind::Tiles:
      out += kTilesSuffix;
      break;
    case RemoteRoute::Kind::Tile:
      out += kTileSuffix;
      out += std::to_string(route.scale) + ',' + std::to_string(route.x) + ',' +
             std::to_string(route.y);
      break;
  }
  return out;
}

std::string remote_query_escape(std::string_view s) {
  return percent_escape(s, "");
}

const char* tile_codec_mime(std::string_view codec) noexcept {
  if (codec == "png") return "image/png";
  if (codec == "jxl") return "image/jxl";
  return "image/jpeg";
}

std::string tile_codec_from_mime(std::string_view mime) {
  if (const auto semi = mime.find(';'); semi != std::string_view::npos) {
    mime = mime.substr(0, semi);
  }
  if (mime == "image/png") return "png";
  if (mime == "image/jxl") return "jxl";
  return "jpeg";
}

std::string base64_encode(const std::vector<std::uint8_t>& data) {
  std::string out;
  out.reserve((data.size() + 2) / 3 * 4);
  std::size_t i = 0;
  for (; i + 2 < data.size(); i += 3) {
    const std::uint32_t v = (data[i] << 16) | (data[i + 1] << 8) | data[i + 2];
    out.push_back(kB64[(v >> 18) & 63]);
    out.push_back(kB64[(v >> 12) & 63]);
    out.push_back(kB64[(v >> 6) & 63]);
    out.push_back(kB64[v & 63]);
  }
  if (i + 1 == data.size()) {
    const std::uint32_t v = data[i] << 16;
    out.push_back(kB64[(v >> 18) & 63]);
    out.push_back(kB64[(v >> 12) & 63]);
    out += "==";
  } else if (i + 2 == data.size()) {
    const std::uint32_t v = (data[i] << 16) | (data[i + 1] << 8);
    out.push_back(kB64[(v >> 18) & 63]);
    out.push_back(kB64[(v >> 12) & 63]);
    out.push_back(kB64[(v >> 6) & 63]);
    out.push_back('=');
  }
  return out;
}

std::optional<std::vector<std::uint8_t>> base64_decode(std::string_view text) {
  std::array<int, 256> rev{};
  rev.fill(-1);
  for (int i = 0; i < 64; ++i) rev[static_cast<unsigned char>(kB64[i])] = i;
  while (!text.empty() && text.back() == '=') text.remove_suffix(1);
  std::vector<std::uint8_t> out;
  out.reserve(text.size() * 3 / 4);
  std::uint32_t acc = 0;
  int bits = 0;
  for (char ch : text) {
    const int v = rev[static_cast<unsigned char>(ch)];
    if (v < 0) return std::nullopt;
    acc = (acc << 6) | static_cast<std::uint32_t>(v);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      out.push_back(static_cast<std::uint8_t>((acc >> bits) & 0xff));
    }
  }
  return out;
}

#if defined(THUMTOO_HAVE_REMOTE)

namespace {

using nlohmann::json;

json info_to_json_value(const RemoteInfo& info) {
  json j;
  j["uri"] = info.uri;
  if (info.outer_path) j["outer_path"] = *info.outer_path;
  if (info.member_path) j["member_path"] = *info.member_path;
  if (info.file_size) j["file_size"] = *info.file_size;
  if (info.mtime_ns) j["mtime_ns"] = *info.mtime_ns;
  if (info.meta) {
    const ContentMeta& m = *info.meta;
    j["content_id"] = m.content_id;
    j["status"] = std::string(to_string(m.status));
    if (m.format) j["format"] = *m.format;
    if (m.size) {
      j["width"] = m.size->width;
      j["height"] = m.size->height;
    }
  }
  if (info.coverage) {
    j["min_scale"] = info.coverage->min_scale;
    j["max_scale"] = info.coverage->max_scale;
  }
  if (info.lqip) j["lqip"] = base64_encode(*info.lqip);
  return j;
}

std::optional<ContentStatus> status_from_string(std::string_view s) {
  for (auto st : {ContentStatus::Pending, ContentStatus::Ready,
                  ContentStatus::Failed, ContentStatus::Unsupported,
                  ContentStatus::Incomplete}) {
    if (to_string(st) == s) return st;
  }
  return std::nullopt;
}

template <typename T>
std::optional<T> opt_field(const json& j, const char* key) {
  auto it = j.find(key);
  if (it == j.end() || it->is_null()) return std::nullopt;
  return it->get<T>();
}

std::optional<RemoteInfo> info_from_json_value(const json& j) {
  if (!j.is_object() || !j.contains("uri")) return std::nullopt;
  RemoteInfo info;
  info.uri = j.at("uri").get<std::string>();
  info.outer_path = opt_field<std::string>(j, "outer_path");
  info.member_path = opt_field<std::string>(j, "member_path");
  info.file_size = opt_field<std::int64_t>(j, "file_size");
  info.mtime_ns = opt_field<std::int64_t>(j, "mtime_ns");
  if (auto cid = opt_field<std::string>(j, "content_id")) {
    ContentMeta m;
    m.content_id = std::move(*cid);
    if (auto st = opt_field<std::string>(j, "status")) {
      if (auto parsed = status_from_string(*st)) m.status = *parsed;
    }
    m.format = opt_field<std::string>(j, "format");
    auto w = opt_field<int>(j, "width");
    auto h = opt_field<int>(j, "height");
    if (w && h) m.size = Size{*w, *h};
    info.meta = std::move(m);
  }
  auto lo = opt_field<int>(j, "min_scale");
  auto hi = opt_field<int>(j, "max_scale");
  if (lo && hi) {
    TileCoverage cov;
    cov.min_scale = *lo;
    cov.max_scale = *hi;
    if (info.meta && info.meta->size) cov.size = *info.meta->size;
    info.coverage = cov;
  }
  if (auto b64 = opt_field<std::string>(j, "lqip")) {
    info.lqip = base64_decode(*b64);
  }
  return info;
}

}  // namespace

std::string remote_info_to_json(const RemoteInfo& info) {
  return info_to_json_value(info).dump();
}

std::optional<RemoteInfo> remote_info_from_json(std::string_view text) {
  try {
    return info_from_json_value(json::parse(text));
  } catch (const json::exception&) {
    return std::nullopt;
  }
}

std::string remote_locations_to_json(const RemoteLocations& page) {
  json items = json::array();
  for (const auto& info : page.items) items.push_back(info_to_json_value(info));
  json j;
  j["locations"] = std::move(items);
  j["next"] = page.next ? json(*page.next) : json(nullptr);
  return j.dump();
}

std::optional<RemoteLocations> remote_locations_from_json(std::string_view text) {
  try {
    const json j = json::parse(text);
    if (!j.is_object() || !j.contains("locations") ||
        !j.at("locations").is_array()) {
      return std::nullopt;
    }
    RemoteLocations page;
    for (const auto& item : j.at("locations")) {
      auto info = info_from_json_value(item);
      if (!info) return std::nullopt;
      page.items.push_back(std::move(*info));
    }
    page.next = opt_field<std::string>(j, "next");
    return page;
  } catch (const json::exception&) {
    return std::nullopt;
  }
}

#else

std::string remote_info_to_json(const RemoteInfo&) { return {}; }
std::optional<RemoteInfo> remote_info_from_json(std::string_view) {
  return std::nullopt;
}
std::string remote_locations_to_json(const RemoteLocations&) { return {}; }
std::optional<RemoteLocations> remote_locations_from_json(std::string_view) {
  return std::nullopt;
}

#endif

}  // namespace thumtoo
