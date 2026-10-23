// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/remote.hpp"
#include "thumtoo/types.hpp"

#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace thumtoo {

/// HTTP side of a remote Client (Client::open_remote): talks to
/// thumtoo-serve and memoizes every RemoteInfo it sees, so the cache-only
/// get_* calls can answer from RAM without blocking on the network.
/// Thread-safe; blocking calls run on Client workers (or once at listing).
class RemoteCache {
 public:
  /// Parses `http(s)://host[:port][/prefix][?token=T]` and checks the server
  /// (`GET /`). Throws std::runtime_error when unreachable, unauthorized or
  /// not a thumtoo-serve of a compatible API version.
  explicit RemoteCache(std::string_view url);
  ~RemoteCache();
  RemoteCache(const RemoteCache&) = delete;
  RemoteCache& operator=(const RemoteCache&) = delete;

  struct Reply {
    long status = 0;  ///< HTTP status; 0 = transport error (see error)
    std::string content_type;
    std::vector<std::uint8_t> body;
    std::string error;
  };
  /// GET base + path_and_query (already escaped).
  [[nodiscard]] Reply get(const std::string& path_and_query) const;

  /// Memo lookup only (never touches the network).
  [[nodiscard]] std::optional<RemoteInfo> cached(std::string_view uri) const;
  void remember(const RemoteInfo& info);

  /// `//info` for one location; memoized. nullopt on 404 or error.
  std::optional<RemoteInfo> fetch_info(const std::string& uri);

  /// One stored tile, decoded to rgb888 like local interactive cells.
  /// 404 → Unavailable (the server never generates tiles).
  [[nodiscard]] TileResult fetch_tile(const std::string& uri, int scale, int x,
                                      int y) const;

  enum class ListFilter { All, Prefix, Like };
  /// Pages through `/locations` until `limit` rows; memoizes every row.
  std::vector<RemoteInfo> list(ListFilter filter, std::string_view value,
                               int limit);

  [[nodiscard]] const std::string& base_url() const { return base_; }

 private:
  void* acquire_handle() const;
  void release_handle(void* handle) const;

  std::string base_;         ///< scheme://host[:port][/prefix], no trailing '/'
  std::string auth_header_;  ///< "Authorization: Bearer …" or empty

  mutable std::mutex memo_mu_;
  std::unordered_map<std::string, RemoteInfo> memo_;

  mutable std::mutex pool_mu_;
  mutable std::vector<void*> handles_;  ///< Idle CURL easy handles (keep-alive)
};

}  // namespace thumtoo
