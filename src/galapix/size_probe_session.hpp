// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_GALAPIX_SIZE_PROBE_SESSION_HPP
#define HEADER_GALAPIX_GALAPIX_SIZE_PROBE_SESSION_HPP

#include <atomic>
#include <memory>
#include <string>
#include <vector>

#include "galapix/image.hpp"

namespace thumtoo { class Client; }

namespace galapix {

/** Background thumtoo size probes while the viewer is already running.
 *
 *  Queue request_size for unknown URIs, open the UI immediately with
 *  placeholder Images, then attach real TileProviders on the main thread
 *  as get_size() becomes available. Optional drain runs on a worker thread.
 */
class SizeProbeSession
{
public:
  SizeProbeSession();

  void add_pending(std::shared_ptr<Image> image, std::string uri);

  /** Number of images still waiting for a size. */
  int remaining() const { return static_cast<int>(m_pending.size()); }
  int total() const { return m_total; }
  int completed() const { return m_completed; }
  bool finished() const { return m_pending.empty() && m_drain_started; }

  /** Start Client::drain() on a background thread (once). Keeps client. */
  void start_drain(std::shared_ptr<thumtoo::Client> client,
                   std::shared_ptr<SizeProbeSession> self);

  /** Main-thread: attach providers for sizes that are ready; update UI. */
  void tick();

private:
  struct Item
  {
    std::weak_ptr<Image> image;
    std::string uri;
  };

  std::vector<Item> m_pending;
  int m_total = 0;
  int m_completed = 0;
  int m_last_logged = -1;
  bool m_drain_started = false;
  bool m_completion_notified = false;
  std::atomic<bool> m_drain_done{false};
  std::shared_ptr<thumtoo::Client> m_client;
};

} // namespace galapix

#endif

/* EOF */
