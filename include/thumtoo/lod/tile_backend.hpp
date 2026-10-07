// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/lod/tile_types.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace thumtoo::lod {

/// Backend outcome for one cell (mirrors thumtoo::TileStatus).
enum class FetchStatus : std::uint8_t {
  Ok,           ///< bitmap holds pixels
  Cancelled,    ///< cancelled before produced; cell may be re-requested
  Failed,       ///< error says why; retried with backoff
  Unavailable,  ///< cell cannot exist; never retried
};

[[nodiscard]] constexpr char const* fetch_status_name(FetchStatus s) noexcept
{
  switch (s) {
  case FetchStatus::Ok:
    return "ok";
  case FetchStatus::Cancelled:
    return "cancelled";
  case FetchStatus::Failed:
    return "failed";
  case FetchStatus::Unavailable:
    return "unavailable";
  }
  return "?";
}

struct FetchRequest {
  TileKey key;
  std::uint64_t ticket = 0;  ///< Echoed back in FetchResult
};

struct FetchResult {
  TileKey key;
  std::uint64_t ticket = 0;
  FetchStatus status = FetchStatus::Failed;
  TileBitmap bitmap;  ///< Ok only
  std::string error;  ///< non-Ok: human-readable reason
};

/**
 * Async tile producer for one path (production: thumtoo request_tile_cells).
 *
 * Contract: `on_result` is called **exactly once per requested cell**, from
 * any thread, including after cancel (FetchStatus::Cancelled). A backend that
 * cannot honour this must synthesize results; the loader's stall watchdog is
 * only a last line of defence and reports a contract violation when it fires.
 */
class TileBackend {
public:
  virtual ~TileBackend() = default;

  using ResultFn = std::function<void(FetchResult)>;

  virtual void fetch(std::vector<FetchRequest> cells, ResultFn on_result) = 0;

  /// Best-effort: cancelled cells must still produce a (Cancelled) result.
  virtual void cancel(std::vector<TileKey> const& keys) = 0;
};

}  // namespace thumtoo::lod

