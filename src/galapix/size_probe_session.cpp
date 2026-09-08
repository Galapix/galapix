// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "galapix/size_probe_session.hpp"

#include <iostream>
#include <thread>

#include <logmich/log.hpp>
#include <thumtoo/client.hpp>

#include "app/imgui_overlay.hpp"
#include "galapix/viewer.hpp"
#include "thumtoo/thumtoo_tile_provider.hpp"

namespace galapix {

SizeProbeSession::SizeProbeSession() = default;

void
SizeProbeSession::add_pending(std::shared_ptr<Image> image, std::string uri)
{
  if (!image || uri.empty()) {
    return;
  }
  m_pending.push_back(Item{std::weak_ptr<Image>(std::move(image)), std::move(uri)});
  ++m_total;
}

void
SizeProbeSession::start_drain(std::shared_ptr<thumtoo::Client> client)
{
  if (m_drain_started || !client) {
    return;
  }
  m_drain_started = true;
  if (m_total == 0) {
    m_drain_done.store(true);
    return;
  }

  std::cout << "Probing image sizes (thumtoo): 0/" << m_total
            << " (viewer open, probing in background)\n"
            << std::flush;

  // ImGui may not be up yet (viewer launches after start_drain); console
  // status is enough until the first tick after the window opens.

  // drain() blocks until queued request_size work finishes. Run it off the
  // GUI thread so the window can open and stay interactive.
  std::thread([client, this]() {
    try {
      client->drain();
    } catch (std::exception const& err) {
      log_error("size probe drain: {}", err.what());
    } catch (...) {
      log_error("size probe drain: unknown error");
    }
    m_drain_done.store(true);
    if (Viewer* v = Viewer::current()) {
      v->redraw();
    }
  }).detach();
}

void
SizeProbeSession::tick(std::shared_ptr<thumtoo::Client> const& client)
{
  if (!client || m_pending.empty()) {
    if (m_drain_done.load() && !m_completion_notified && m_total > 0) {
      m_completion_notified = true;
      std::cout << "Probing image sizes (thumtoo): done "
                << m_completed << "/" << m_total << "\n"
                << std::flush;
      ImguiOverlay::notify(
        "Size probe complete (" + std::to_string(m_completed) + "/" +
          std::to_string(m_total) + ")",
        4.0f);
      if (Viewer* v = Viewer::current()) {
        v->layout_tight();
        v->zoom_to_selection();
        v->redraw();
      }
    }
    return;
  }

  std::vector<Item> still;
  still.reserve(m_pending.size());

  for (auto& item : m_pending) {
    auto img = item.image.lock();
    if (!img) {
      continue;
    }
    auto sz = client->get_size(item.uri);
    if (!sz || sz->width <= 0 || sz->height <= 0) {
      still.push_back(std::move(item));
      continue;
    }
    auto provider = ThumtooTileProvider::create_from_size(
      client, item.uri, sz->width, sz->height);
    if (provider) {
      img->set_tile_provider(std::move(provider));
      ++m_completed;
    } else {
      still.push_back(std::move(item));
    }
  }
  m_pending.swap(still);

  if (!m_pending.empty()) {
    if (Viewer* v = Viewer::current()) {
      v->redraw();
    }
  }

  int const done = m_completed;
  int const tot = m_total;
  // Log / toast every 8 completions or when finished.
  if (done != m_last_logged &&
      (done == tot || done - m_last_logged >= 8 || done == 1)) {
    m_last_logged = done;
    std::cout << "Probing image sizes (thumtoo): " << done << "/" << tot
              << "\r" << std::flush;
    ImguiOverlay::notify(
      "Probing image sizes: " + std::to_string(done) + "/" + std::to_string(tot),
      2.5f);
    if (Viewer* v = Viewer::current()) {
      v->redraw();
    }
  }

  if (m_pending.empty() && m_drain_done.load() && !m_completion_notified) {
    m_completion_notified = true;
    std::cout << "\nProbing image sizes (thumtoo): done " << m_completed
              << "/" << m_total << "\n"
              << std::flush;
    ImguiOverlay::notify(
      "Size probe complete (" + std::to_string(m_completed) + "/" +
        std::to_string(m_total) + ")",
      4.0f);
    if (Viewer* v = Viewer::current()) {
      v->layout_tight();
      v->zoom_to_selection();
      v->redraw();
    }
  }
}

} // namespace galapix

/* EOF */
