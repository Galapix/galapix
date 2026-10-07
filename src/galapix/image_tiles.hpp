// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_GALAPIX_IMAGE_TILES_HPP
#define HEADER_GALAPIX_GALAPIX_IMAGE_TILES_HPP

#include <chrono>
#include <map>
#include <memory>
#include <string>

#include <thumtoo/lod/tile_session.hpp>
#include <wstdisplay/fwd.hpp>

#include "galapix/texture_surface.hpp"
#include "galapix/tile_provider.hpp"
#include "math/rect.hpp"

namespace galapix {

/** The tiles of one Image on screen, on top of thumtoo::lod (thumtoo
    docs/TILE_LOD.md): a TileSession on the TileLoader shared by all images
    of the same source, the textures of its Ready cells and the LQIP
    placeholder.

    The loader owns every request and cell state, ImageTiles only
    publishes what is visible (update()) and draws the session's draw
    plan (draw()). */
class ImageTiles final
{
public:
  explicit ImageTiles(TileProviderPtr const& provider);
  ~ImageTiles();

  /** Visible: plan + demand for the part of \a image_rect inside \a
      cliprect at view \a zoom, upload textures for the draw plan. */
  void update(wstdisplay::Device& device, Rectf const& image_rect, float image_scale,
              Rectf const& cliprect, float zoom);

  /** Not visible anymore: withdraw the demand (queued cells get cancelled). */
  void hide();

  void draw(wstdisplay::Canvas& canvas, Rectf const& image_rect, float image_scale) const;

  /** Drop every cell and texture, cancel outstanding requests. */
  void clear();

  /** Drop the GPU textures (cells stay with the loader). */
  void release_textures();

  struct Stats
  {
    int cells = 0;            ///< cells known to the loader (any state)
    int queued = 0;           ///< outstanding backend requests
    int ready = 0;            ///< cells with pixels
    int textures = 0;         ///< uploaded cells
    int pending_uploads = 0;  ///< drawable cells still waiting for upload
    thumtoo::lod::TileSession::Phase phase = thumtoo::lod::TileSession::Phase::Idle;
  };
  Stats stats() const;

  /** One line: phase, coverage and the first failure reason. */
  std::string status_line() const;

  /** Tile debug overlay: tint per draw kind and pyramid scale (key 'v'). */
  static void set_tile_debug(bool on) { s_tile_debug = on; }
  static bool tile_debug() { return s_tile_debug; }

  /** Cache-only mode (key 'u'): sessions stay passive, no new requests. */
  static void set_requests_enabled(bool on) { s_requests_enabled = on; }
  static bool requests_enabled() { return s_requests_enabled; }

  /** Call once per frame before the images update: texture uploads per
      frame are capped across all images. */
  static void begin_frame(int max_uploads);
  static int frame_uploads() { return s_frame_uploads; }

private:
  void upload_textures(wstdisplay::Device& device);
  void poll_lqip(wstdisplay::Device& device);
  TextureSurfacePtr texture_for(thumtoo::lod::TileKey const& key) const;

private:
  TileProviderPtr m_provider;
  std::shared_ptr<thumtoo::lod::TileLoader> m_loader;
  std::unique_ptr<thumtoo::lod::TileSession> m_session;
  thumtoo::lod::DrawPlan m_plan;
  std::map<thumtoo::lod::TileKey, TextureSurfacePtr> m_textures;
  TextureSurfacePtr m_lqip;
  std::chrono::steady_clock::time_point m_next_lqip_try;
  bool m_visible;

  static bool s_tile_debug;
  static bool s_requests_enabled;
  static int s_frame_uploads;
  static int s_frame_upload_budget;

public:
  ImageTiles(ImageTiles const&) = delete;
  ImageTiles& operator=(ImageTiles const&) = delete;
};

} // namespace galapix

#endif

/* EOF */
