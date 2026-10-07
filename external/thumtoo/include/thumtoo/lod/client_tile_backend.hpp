// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "thumtoo/lod/tile_backend.hpp"
#include "thumtoo/types.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace thumtoo {
class Client;
}

namespace thumtoo::lod {

/**
 * TileBackend for one URI on a thumtoo::Client, via `request_tile_cells` /
 * `cancel_tile_cells` (TILES.md "Interactive cell contract": exactly one
 * result per cell, Cancelled included).
 *
 * Ok cells are decoded to RGBA8 (`TileBitmap::codec == "rgba8"`) on the
 * thread that delivers the result (the Client Executor), so hosts upload
 * pixels without decoding anything themselves.
 */
class ClientTileBackend : public TileBackend {
public:
  ClientTileBackend(std::shared_ptr<Client> client, std::string uri);

  void fetch(std::vector<FetchRequest> cells, ResultFn on_result) override;
  void cancel(std::vector<TileKey> const& keys) override;

  std::string const& uri() const { return m_uri; }

private:
  std::shared_ptr<Client> m_client;
  std::string m_uri;
};

/// Decode a tile payload (durable "jpeg", live "rgb888", or "rgba8") to an
/// RGBA8 TileBitmap. nullopt when the payload cannot be decoded.
[[nodiscard]] std::optional<TileBitmap> decode_tile_bitmap(TileBlob blob);

}  // namespace thumtoo::lod
