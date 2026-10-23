// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "remote_cache.hpp"

#include "thumtoo/image.hpp"

#include <algorithm>
#include <stdexcept>

#if defined(THUMTOO_HAVE_REMOTE)
#include <curl/curl.h>
#include <nlohmann/json.hpp>
#endif

namespace thumtoo {

#if defined(THUMTOO_HAVE_REMOTE)

namespace {

/// Rows per `/locations` request; the server caps it as well.
constexpr int kListPageSize = 2000;

std::size_t write_cb(char* ptr, std::size_t size, std::size_t nmemb,
                     void* userdata) {
  auto* out = static_cast<std::vector<std::uint8_t>*>(userdata);
  const std::size_t n = size * nmemb;
  out->insert(out->end(), reinterpret_cast<const std::uint8_t*>(ptr),
              reinterpret_cast<const std::uint8_t*>(ptr) + n);
  return n;
}

std::string body_text(const RemoteCache::Reply& r) {
  return std::string(r.body.begin(), r.body.end());
}

std::string describe_failure(const RemoteCache::Reply& r) {
  if (r.status == 0) return r.error;
  std::string msg = "HTTP " + std::to_string(r.status);
  if (!r.body.empty() && r.body.size() < 512) msg += ": " + body_text(r);
  while (!msg.empty() && (msg.back() == '\n' || msg.back() == '\r')) {
    msg.pop_back();
  }
  return msg;
}

}  // namespace

RemoteCache::RemoteCache(std::string_view url) {
  static std::once_flag curl_once;
  std::call_once(curl_once, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });

  if (!is_remote_cache_url(url)) {
    throw std::runtime_error("not a thumtoo-serve URL: " + std::string(url));
  }
  std::string_view base = url;
  std::string token;
  if (const auto q = url.find('?'); q != std::string_view::npos) {
    base = url.substr(0, q);
    std::string_view query = url.substr(q + 1);
    while (!query.empty()) {
      const auto amp = query.find('&');
      const std::string_view kv = query.substr(0, amp);
      if (kv.starts_with("token=")) token = std::string(kv.substr(6));
      if (amp == std::string_view::npos) break;
      query.remove_prefix(amp + 1);
    }
  }
  while (base.ends_with('/')) base.remove_suffix(1);
  base_ = std::string(base);
  if (!token.empty()) auth_header_ = "Authorization: Bearer " + token;

  const Reply hello = get("/");
  if (hello.status != 200) {
    throw std::runtime_error("thumtoo-serve " + base_ + ": " +
                             describe_failure(hello));
  }
  try {
    const auto j = nlohmann::json::parse(body_text(hello));
    if (j.value("service", std::string()) != "thumtoo-serve") {
      throw std::runtime_error("not a thumtoo-serve: " + base_);
    }
    const int api = j.value("api", 0);
    if (api != kRemoteApiVersion) {
      throw std::runtime_error("thumtoo-serve " + base_ + " speaks API " +
                               std::to_string(api) + ", this build " +
                               std::to_string(kRemoteApiVersion));
    }
  } catch (const nlohmann::json::exception&) {
    throw std::runtime_error("not a thumtoo-serve (bad handshake): " + base_);
  }
}

RemoteCache::~RemoteCache() {
  for (void* h : handles_) curl_easy_cleanup(static_cast<CURL*>(h));
}

void* RemoteCache::acquire_handle() const {
  {
    std::lock_guard lock(pool_mu_);
    if (!handles_.empty()) {
      void* h = handles_.back();
      handles_.pop_back();
      return h;
    }
  }
  return curl_easy_init();
}

void RemoteCache::release_handle(void* handle) const {
  std::lock_guard lock(pool_mu_);
  handles_.push_back(handle);
}

RemoteCache::Reply RemoteCache::get(const std::string& path_and_query) const {
  Reply r;
  CURL* curl = static_cast<CURL*>(acquire_handle());
  if (!curl) {
    r.error = "curl_easy_init failed";
    return r;
  }
  curl_easy_reset(curl);  // keeps the connection cache
  const std::string url = base_ + path_and_query;
  curl_slist* headers = nullptr;
  if (!auth_header_.empty()) {
    headers = curl_slist_append(headers, auth_header_.c_str());
  }
  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, "thumtoo-remote/1");
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &r.body);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);
  curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);
  const CURLcode rc = curl_easy_perform(curl);
  if (rc != CURLE_OK) {
    r.error = std::string("GET ") + url + ": " + curl_easy_strerror(rc);
    r.body.clear();
  } else {
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &r.status);
    char* ct = nullptr;
    if (curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &ct) == CURLE_OK && ct) {
      r.content_type = ct;
    }
  }
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, nullptr);
  curl_slist_free_all(headers);
  release_handle(curl);
  return r;
}

std::optional<RemoteInfo> RemoteCache::cached(std::string_view uri) const {
  std::lock_guard lock(memo_mu_);
  auto it = memo_.find(std::string(uri));
  if (it == memo_.end()) return std::nullopt;
  return it->second;
}

void RemoteCache::remember(const RemoteInfo& info) {
  std::lock_guard lock(memo_mu_);
  memo_[info.uri] = info;
}

std::optional<RemoteInfo> RemoteCache::fetch_info(const std::string& uri) {
  RemoteRoute route;
  route.kind = RemoteRoute::Kind::Info;
  route.location = uri;
  const Reply r = get(format_remote_route(route));
  if (r.status != 200) return std::nullopt;
  auto info = remote_info_from_json(body_text(r));
  if (info) remember(*info);
  return info;
}

TileResult RemoteCache::fetch_tile(const std::string& uri, int scale, int x,
                                   int y) const {
  RemoteRoute route;
  route.kind = RemoteRoute::Kind::Tile;
  route.location = uri;
  route.scale = scale;
  route.x = x;
  route.y = y;
  Reply r = get(format_remote_route(route));
  if (r.status == 404) {
    return {TileStatus::Unavailable, std::nullopt,
            "not in the remote cache (" + base_ + ")"};
  }
  if (r.status != 200) {
    return {TileStatus::Failed, std::nullopt, describe_failure(r)};
  }
  TileBlob blob;
  blob.scale = scale;
  blob.x = x;
  blob.y = y;
  blob.codec = tile_codec_from_mime(r.content_type);
  blob.bytes = std::move(r.body);
  auto rgb = decode_tile_blob_to_rgb888(std::move(blob));
  if (!rgb) {
    return {TileStatus::Failed, std::nullopt, "remote tile could not be decoded"};
  }
  return {TileStatus::Ok, std::move(rgb), {}};
}

std::vector<RemoteInfo> RemoteCache::list(ListFilter filter,
                                          std::string_view value, int limit) {
  std::vector<RemoteInfo> out;
  std::string after;
  while (limit <= 0 || static_cast<int>(out.size()) < limit) {
    const int want =
        limit <= 0 ? kListPageSize
                   : std::min(kListPageSize, limit - static_cast<int>(out.size()));
    std::string q = "/locations?limit=" + std::to_string(want);
    if (filter == ListFilter::Prefix) {
      q += "&prefix=" + remote_query_escape(value);
    } else if (filter == ListFilter::Like) {
      q += "&like=" + remote_query_escape(value);
    }
    if (!after.empty()) q += "&after=" + remote_query_escape(after);
    const Reply r = get(q);
    if (r.status != 200) {
      throw std::runtime_error("thumtoo-serve /locations: " + describe_failure(r));
    }
    auto page = remote_locations_from_json(body_text(r));
    if (!page) throw std::runtime_error("thumtoo-serve /locations: bad JSON");
    {
      std::lock_guard lock(memo_mu_);
      for (const auto& info : page->items) memo_[info.uri] = info;
    }
    for (auto& info : page->items) out.push_back(std::move(info));
    if (!page->next) break;
    after = *page->next;
  }
  return out;
}

#else  // !THUMTOO_HAVE_REMOTE

RemoteCache::RemoteCache(std::string_view) {
  throw std::runtime_error(
      "thumtoo was built without remote support (needs libcurl and "
      "nlohmann_json)");
}
RemoteCache::~RemoteCache() = default;
void* RemoteCache::acquire_handle() const { return nullptr; }
void RemoteCache::release_handle(void*) const {}
RemoteCache::Reply RemoteCache::get(const std::string&) const { return {}; }
std::optional<RemoteInfo> RemoteCache::cached(std::string_view) const {
  return std::nullopt;
}
void RemoteCache::remember(const RemoteInfo&) {}
std::optional<RemoteInfo> RemoteCache::fetch_info(const std::string&) {
  return std::nullopt;
}
TileResult RemoteCache::fetch_tile(const std::string&, int, int, int) const {
  return {TileStatus::Failed, std::nullopt, "no remote support"};
}
std::vector<RemoteInfo> RemoteCache::list(ListFilter, std::string_view, int) {
  return {};
}

#endif

}  // namespace thumtoo
