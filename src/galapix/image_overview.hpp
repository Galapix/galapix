// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_GALAPIX_IMAGE_OVERVIEW_HPP
#define HEADER_GALAPIX_GALAPIX_IMAGE_OVERVIEW_HPP

#include <memory>
#include <optional>

#include <surf/software_surface.hpp>
#include <wstdisplay/surface.hpp>

#include "galapix/tile_provider.hpp"
#include "job/job_handle.hpp"
#include "job/thread_message_queue2.hpp"
#include "math/rect.hpp"
#include "util/url.hpp"

namespace galapix {

class JobManager;

/** In-memory soft whole-image preview (not a grid tile).
 *
 *  See docs/OVERVIEW.md. Loaded via OverviewLoadJob / libjpeg scale.
 */
class ImageOverview : public std::enable_shared_from_this<ImageOverview>
{
public:
  enum class State
  {
    Idle,
    Loading,
    Ready,
    Failed
  };

  ImageOverview() = default;

  State state() const { return m_state; }
  bool has_surface() const { return static_cast<bool>(m_surface); }

  /** Start load once if Idle.
   *  Prefer thumtoo max_scale tile when \a provider is ThumtooTileProvider
   *  (archives / warm cache). Else JobManager + TileGenerator (local files). */
  void ensure_requested(JobManager* job_manager, URL const& url,
                        int original_width, int original_height,
                        TileProviderPtr provider = {});

  /** Main thread: promote decoded software surface to a GL texture. */
  void process();

  /** Draw stretched to the image rect (under tiles). */
  void draw(wstdisplay::GraphicsContext& gc, Rectf const& image_rect);

  void clear();

private:
  void receive_software(std::optional<surf::SoftwareSurface> surface);

  State m_state = State::Idle;
  JobHandle m_job = JobHandle::create();
  ThreadMessageQueue2<std::optional<surf::SoftwareSurface>> m_queue;
  wstdisplay::SurfacePtr m_surface;
};

} // namespace galapix

#endif

/* EOF */
