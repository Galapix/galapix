// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "galapix/image_tiles.hpp"

#include <algorithm>
#include <cstring>

#include <logmich/log.hpp>
#include <surf/color.hpp>
#include <surf/pixel_format.hpp>
#include <surf/software_surface.hpp>
#include <thumtoo/client.hpp>
#include <thumtoo/lqip.hpp>
#include <wstdisplay/canvas.hpp>
#include <wstdisplay/device.hpp>

#include "galapix/tile_lod.hpp"
#include "galapix/viewer.hpp"
#include "thumtoo/thumtoo_tile_provider.hpp"

namespace galapix {

namespace lod = thumtoo::lod;

bool ImageTiles::s_tile_debug = false;
bool ImageTiles::s_requests_enabled = true;
int ImageTiles::s_frame_uploads = 0;
int ImageTiles::s_frame_upload_budget = 0;

namespace {

/** Retry an LQIP that is not in the thumtoo cache yet (it is written
    while tiles are encoded) */
constexpr auto kLqipRetry = std::chrono::milliseconds(500);

surf::Color const kPlaceholderColor = surf::Color::from_rgb888(155, 0, 155);

TextureSurfacePtr
texture_from_rgba(wstdisplay::Device& device, int width, int height, std::uint8_t const* rgba)
{
  surf::SoftwareSurface image = surf::SoftwareSurface::create(surf::PixelFormat::RGBA8,
                                                              geom::isize(width, height));
  size_t const row_bytes = static_cast<size_t>(width) * 4;
  for (int y = 0; y < height; ++y) {
    std::memcpy(image.get_row_data(y), rgba + row_bytes * static_cast<size_t>(y), row_bytes);
  }

  // Exact size, clamped: linear filtering never samples outside the cell.
  wstdisplay::Unique<wstdisplay::Texture> texture =
    device.create_texture(image, {.filter = wstdisplay::TextureFilter::Linear});
  wstdisplay::Surface const surface(texture, image.get_size());
  return std::make_shared<TextureSurface const>(std::move(texture), surface);
}

Rectf
to_world(lod::RectF const& content, Rectf const& image_rect, float image_scale)
{
  return Rectf(geom::fpoint(image_rect.left() + static_cast<float>(content.x) * image_scale,
                            image_rect.top() + static_cast<float>(content.y) * image_scale),
               geom::fsize(static_cast<float>(content.w) * image_scale,
                           static_cast<float>(content.h) * image_scale));
}

Rectf
to_rectf(lod::RectF const& r)
{
  return Rectf(geom::fpoint(static_cast<float>(r.x), static_cast<float>(r.y)),
               geom::fsize(static_cast<float>(r.w), static_cast<float>(r.h)));
}

/** Semi-transparent tint for the pyramid scale of the drawn pixels.
    Successive scales get distinct hues; the exact target scale is pulled
    toward green. */
surf::Color
debug_fill_for_scale(int tile_scale, int target_scale)
{
  static constexpr uint8_t kPalette[][3] = {
    {  40, 200,  80 }, // 0 green
    {  60, 180, 220 }, // 1 cyan
    {  80, 120, 255 }, // 2 blue
    { 160, 100, 255 }, // 3 violet
    { 220,  80, 200 }, // 4 magenta
    { 240,  90,  90 }, // 5 red
    { 240, 150,  50 }, // 6 orange
    { 230, 210,  40 }, // 7 yellow
    { 160, 220,  50 }, // 8 lime
    {  50, 210, 160 }, // 9 teal
    { 100, 160, 240 }, // 10 light blue
    { 200, 120, 160 }, // 11 rose
  };
  constexpr int n = static_cast<int>(sizeof(kPalette) / sizeof(kPalette[0]));
  int idx = tile_scale % n;
  if (idx < 0) {
    idx += n;
  }

  uint8_t r = kPalette[idx][0];
  uint8_t g = kPalette[idx][1];
  uint8_t b = kPalette[idx][2];

  if (tile_scale == target_scale) {
    r = static_cast<uint8_t>(r / 2);
    g = static_cast<uint8_t>(std::min(255, g + 40));
    b = static_cast<uint8_t>(b / 2);
  }

  return surf::Color::from_rgba8888(r, g, b, 90);
}

void
draw_debug_overlay(wstdisplay::Canvas& canvas, Rectf const& rect, lod::DrawCommand const& cmd,
                   int target_scale, float line_width)
{
  switch (cmd.kind)
  {
    case lod::DrawKind::ExactTile:
    case lod::DrawKind::CoarserTile:
      {
        surf::Color const fill = debug_fill_for_scale(cmd.src_key.scale, target_scale);
        canvas.fill_rect(rect, fill);
        canvas.draw_rect(rect, surf::Color::from_rgba8888(fill.r8(), fill.g8(), fill.b8(), 255), line_width);
      }
      break;

    case lod::DrawKind::Underlay:
    case lod::DrawKind::Empty:
      canvas.fill_rect(rect, surf::Color::from_rgba8888(155, 0, 155, 70));
      canvas.draw_rect(rect, kPlaceholderColor, line_width);
      break;
  }
}

} // namespace

ImageTiles::ImageTiles(TileProviderPtr const& provider) :
  m_provider(provider),
  m_loader(tile_lod::loader_for(*provider)),
  m_session(std::make_unique<lod::TileSession>(m_loader)),
  m_plan(),
  m_textures(),
  m_lqip(),
  m_next_lqip_try(),
  m_visible(false)
{
  m_session->set_content_size(provider->get_size().width(), provider->get_size().height(),
                              provider->get_min_scale());
  m_session->set_on_change([] {
    if (Viewer* viewer = Viewer::current()) {
      viewer->redraw();
    }
  });
}

ImageTiles::~ImageTiles()
{
}

void
ImageTiles::begin_frame(int max_uploads)
{
  s_frame_uploads = 0;
  s_frame_upload_budget = max_uploads;
}

void
ImageTiles::update(wstdisplay::Device& device, Rectf const& image_rect, float image_scale,
                   Rectf const& cliprect, float zoom)
{
  if (!geom::intersects(image_rect, cliprect) || image_scale <= 0.0f) {
    hide();
    return;
  }

  Rectf const visible = geom::intersection(image_rect, cliprect);
  lod::Viewport viewport;
  viewport.content_rect = {
    static_cast<double>((visible.left() - image_rect.left()) / image_scale),
    static_cast<double>((visible.top() - image_rect.top()) / image_scale),
    static_cast<double>(visible.width() / image_scale),
    static_cast<double>(visible.height() / image_scale)
  };
  viewport.device_per_content = static_cast<double>(zoom * image_scale);

  m_session->set_passive(!s_requests_enabled);
  m_session->set_viewport(viewport);
  m_visible = true;

  poll_lqip(device);
  m_plan = m_session->draw_plan();
  upload_textures(device);
}

void
ImageTiles::hide()
{
  if (m_visible) {
    m_visible = false;
    m_session->clear_demand();
  }
}

void
ImageTiles::upload_textures(wstdisplay::Device& device)
{
  // Textures follow the loader: evicted or invalidated cells go.
  for (auto it = m_textures.begin(); it != m_textures.end();) {
    lod::TileCell const* cell = m_loader->find(it->first);
    it = (cell && cell->ready()) ? std::next(it) : m_textures.erase(it);
  }

  for (lod::DrawCommand const& cmd : m_plan.commands)
  {
    if (cmd.kind != lod::DrawKind::ExactTile && cmd.kind != lod::DrawKind::CoarserTile) {
      continue;
    }

    m_loader->touch(cmd.src_key);
    if (m_textures.count(cmd.src_key) || s_frame_upload_budget <= 0) {
      continue;
    }

    lod::TileCell const* cell = m_loader->find(cmd.src_key);
    if (!cell || !cell->ready() || cell->bitmap.codec != "rgba8") {
      continue;
    }

    m_textures[cmd.src_key] = texture_from_rgba(device, cell->bitmap.width, cell->bitmap.height,
                                                cell->bitmap.bytes.data());
    s_frame_upload_budget -= 1;
    s_frame_uploads += 1;
  }

  if (s_frame_upload_budget <= 0) {
    // Keep frames coming until the backlog is uploaded.
    if (Viewer* viewer = Viewer::current()) {
      viewer->redraw();
    }
  }
}

void
ImageTiles::poll_lqip(wstdisplay::Device& device)
{
  if (m_lqip) {
    return;
  }

  auto const* provider = dynamic_cast<ThumtooTileProvider const*>(m_provider.get());
  if (!provider) {
    return;
  }

  auto const now = std::chrono::steady_clock::now();
  if (now < m_next_lqip_try) {
    return;
  }
  m_next_lqip_try = now + kLqipRetry;

  auto const hash = provider->client()->get_lqip(provider->uri());
  if (!hash) {
    return;
  }

  auto const img = thumtoo::lqip_decode_rgba(
    std::span<std::uint8_t const>(hash->data(), hash->size()));
  if (!img || img->width <= 0 || img->height <= 0 ||
      img->rgba.size() < static_cast<size_t>(img->width) * static_cast<size_t>(img->height) * 4) {
    log_debug("ImageTiles: LQIP decode failed for {}", provider->uri());
    return;
  }

  m_lqip = texture_from_rgba(device, img->width, img->height, img->rgba.data());
  m_session->set_has_lqip(true);
}

TextureSurfacePtr
ImageTiles::texture_for(lod::TileKey const& key) const
{
  auto it = m_textures.find(key);
  return it != m_textures.end() ? it->second : TextureSurfacePtr();
}

void
ImageTiles::draw(wstdisplay::Canvas& canvas, Rectf const& image_rect, float image_scale,
                 float zoom) const
{
  int const content_w = m_session->content_w();
  int const content_h = m_session->content_h();
  if (content_w <= 0 || content_h <= 0) {
    return;
  }

  // Nothing to show yet: placeholder for the whole image.
  if (!m_plan.any_tile && !m_lqip) {
    canvas.fill_rect(image_rect, kPlaceholderColor);
  }

  auto draw_lqip = [&](lod::RectF const& content, Rectf const& dst) {
    if (!m_lqip) {
      return;
    }
    float const sx = m_lqip->get_width() / static_cast<float>(content_w);
    float const sy = m_lqip->get_height() / static_cast<float>(content_h);
    canvas.draw(m_lqip->get().region(Rectf(geom::fpoint(static_cast<float>(content.x) * sx,
                                                        static_cast<float>(content.y) * sy),
                                           geom::fsize(static_cast<float>(content.w) * sx,
                                                       static_cast<float>(content.h) * sy))),
                dst);
  };

  for (lod::DrawCommand const& cmd : m_plan.commands)
  {
    Rectf const dst = to_world(cmd.dst_content, image_rect, image_scale);

    switch (cmd.kind)
    {
      case lod::DrawKind::ExactTile:
      case lod::DrawKind::CoarserTile:
        if (TextureSurfacePtr const texture = texture_for(cmd.src_key)) {
          canvas.draw(texture->get().region(to_rectf(cmd.src_uv)), dst);
        } else {
          // Ready but not uploaded yet (per-frame upload budget)
          draw_lqip(cmd.dst_content, dst);
        }
        break;

      case lod::DrawKind::Underlay:
        draw_lqip(cmd.dst_content, dst);
        break;

      case lod::DrawKind::Empty:
        break;
    }

    if (s_tile_debug) {
      draw_debug_overlay(canvas, dst, cmd, m_plan.target_scale, 1.0f / zoom);
    }
  }
}

void
ImageTiles::clear()
{
  m_textures.clear();
  m_lqip.reset();
  m_plan = {};
  m_next_lqip_try = {};
  m_session->set_has_lqip(false);
  m_loader->invalidate("cache cleared");
}

void
ImageTiles::release_textures()
{
  m_textures.clear();
  m_lqip.reset();
  m_next_lqip_try = {};
  m_session->set_has_lqip(false);
}

ImageTiles::Stats
ImageTiles::stats() const
{
  Stats stats;
  stats.cells = static_cast<int>(m_loader->cells().size());
  stats.queued = m_loader->queued_count();
  stats.ready = static_cast<int>(m_loader->ready_count());
  stats.textures = static_cast<int>(m_textures.size());
  for (lod::DrawCommand const& cmd : m_plan.commands) {
    if ((cmd.kind == lod::DrawKind::ExactTile || cmd.kind == lod::DrawKind::CoarserTile) &&
        !m_textures.count(cmd.src_key)) {
      stats.pending_uploads += 1;
    }
  }
  stats.phase = m_session->phase();
  return stats;
}

std::string
ImageTiles::status_line() const
{
  return m_session->status_line();
}

} // namespace galapix

/* EOF */
