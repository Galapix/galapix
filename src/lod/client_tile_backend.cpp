// SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later

#include "thumtoo/lod/client_tile_backend.hpp"

#include "thumtoo/client.hpp"
#include "thumtoo/constants.hpp"
#include "thumtoo/image.hpp"

#include <cstddef>
#include <utility>

namespace thumtoo::lod {

std::optional<TileBitmap> decode_tile_bitmap(TileBlob blob)
{
  if (blob.width <= 0 || blob.height <= 0 || blob.bytes.empty()) {
    return std::nullopt;
  }

  std::size_t const pixels =
      static_cast<std::size_t>(blob.width) * static_cast<std::size_t>(blob.height);

  if (blob.codec == "rgba8") {
    if (blob.bytes.size() < pixels * 4) {
      return std::nullopt;
    }
    return TileBitmap{blob.width, blob.height, "rgba8", std::move(blob.bytes)};
  }

  if (blob.codec != kTileCodecRgb888) {
    auto rgb = decode_tile_blob_to_rgb888(std::move(blob));
    if (!rgb || rgb->codec != kTileCodecRgb888) {
      return std::nullopt;
    }
    blob = std::move(*rgb);
  }

  if (blob.bytes.size() < pixels * 3) {
    return std::nullopt;
  }

  TileBitmap out{blob.width, blob.height, "rgba8", {}};
  out.bytes.resize(pixels * 4);
  std::uint8_t const* src = blob.bytes.data();
  std::uint8_t* dst = out.bytes.data();
  for (std::size_t i = 0; i < pixels; ++i) {
    dst[i * 4 + 0] = src[i * 3 + 0];
    dst[i * 4 + 1] = src[i * 3 + 1];
    dst[i * 4 + 2] = src[i * 3 + 2];
    dst[i * 4 + 3] = 255;
  }
  return out;
}

ClientTileBackend::ClientTileBackend(std::shared_ptr<Client> client, std::string uri) :
  m_client(std::move(client)),
  m_uri(std::move(uri))
{
}

void ClientTileBackend::fetch(std::vector<FetchRequest> cells, ResultFn on_result)
{
  if (cells.empty() || !on_result) {
    return;
  }

  if (!m_client) {
    for (FetchRequest const& c : cells) {
      on_result(FetchResult{c.key, c.ticket, FetchStatus::Failed, {}, "no thumtoo client"});
    }
    return;
  }

  std::vector<Client::TileCoord> coords;
  coords.reserve(cells.size());
  for (FetchRequest const& c : cells) {
    coords.push_back(Client::TileCoord{c.key.scale, c.key.x, c.key.y});
  }

  auto reqs = std::make_shared<std::vector<FetchRequest>>(std::move(cells));
  m_client->request_tile_cells(
      m_uri, std::move(coords),
      [reqs, on_result = std::move(on_result)](std::size_t index, TileResult r) {
        if (index >= reqs->size()) {
          return;
        }
        FetchResult out;
        out.key = (*reqs)[index].key;
        out.ticket = (*reqs)[index].ticket;
        out.error = std::move(r.error);
        switch (r.status) {
        case TileStatus::Ok:
          if (r.tile) {
            if (auto bitmap = decode_tile_bitmap(std::move(*r.tile))) {
              out.status = FetchStatus::Ok;
              out.bitmap = std::move(*bitmap);
              break;
            }
          }
          out.status = FetchStatus::Failed;
          out.error = "tile payload could not be decoded";
          break;
        case TileStatus::Cancelled:
          out.status = FetchStatus::Cancelled;
          break;
        case TileStatus::Failed:
          out.status = FetchStatus::Failed;
          break;
        case TileStatus::Unavailable:
          out.status = FetchStatus::Unavailable;
          break;
        }
        on_result(std::move(out));
      });
}

void ClientTileBackend::cancel(std::vector<TileKey> const& keys)
{
  if (keys.empty() || !m_client) {
    return;
  }
  std::vector<Client::TileCoord> coords;
  coords.reserve(keys.size());
  for (TileKey const& k : keys) {
    coords.push_back(Client::TileCoord{k.scale, k.x, k.y});
  }
  (void)m_client->cancel_tile_cells(m_uri, coords);
}

}  // namespace thumtoo::lod
