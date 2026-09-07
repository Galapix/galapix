// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_THUMTOO_THUMTOO_TILE_PROVIDER_HPP
#define HEADER_GALAPIX_THUMTOO_THUMTOO_TILE_PROVIDER_HPP

#include "galapix/tile_provider.hpp"
#include "math/size.hpp"

#include <memory>
#include <string>

namespace thumtoo {
class Client;
}

namespace galapix {

/** TileProvider backed by thumtoo's Galapix-compatible 256² JPEG pyramid.
 *
 *  Requires HAVE_THUMTOO / WITH_THUMTOO. See thumtoo INTEGRATION_GALAPIX.md.
 *
 *  Does not use Galapix's SQLite tiles table; content is keyed by thumtoo URI
 *  (file:///… or //archive:member). Size and max_scale come from thumtoo
 *  get_size / get_tile_coverage.
 */
class ThumtooTileProvider : public TileProvider
{
public:
  /** Probe size if needed, then construct. Returns empty shared_ptr on failure.
   *  Avoid calling in a tight loop — each miss does request_size + drain(). */
  static TileProviderPtr create(std::shared_ptr<thumtoo::Client> client,
                                std::string uri);

  /** Construct when size is already known (after a batched size probe). */
  static TileProviderPtr create_from_size(std::shared_ptr<thumtoo::Client> client,
                                          std::string uri,
                                          int width, int height);

  ThumtooTileProvider(std::shared_ptr<thumtoo::Client> client, std::string uri,
                      Size size, int max_scale);

  JobHandle request_tile(int tilescale, Vector2i const& pos,
                         const std::function<void(Tile)>& callback) override;

  int get_max_scale() const override { return m_max_scale; }
  int get_tilesize() const override { return 256; }
  Size get_size() const override { return m_size; }

private:
  std::shared_ptr<thumtoo::Client> m_client;
  std::string m_uri;
  Size m_size;
  int m_max_scale;

private:
  ThumtooTileProvider(ThumtooTileProvider const&) = delete;
  ThumtooTileProvider& operator=(ThumtooTileProvider const&) = delete;
};

} // namespace galapix

#endif

/* EOF */
