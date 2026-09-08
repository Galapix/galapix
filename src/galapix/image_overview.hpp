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

/** Soft whole-image underlay: LQIP (ThumbHash) then levels / libjpeg overview.
 *
 *  See GLOSSARY.md (overview / levels / LQIP).
 */
class ImageOverview : public std::enable_shared_from_this<ImageOverview>
{
public:
  /** Load lifecycle (orthogonal to what is drawn). */
  enum class State
  {
    Idle,
    Loading,  ///< job in flight (LQIP, levels, or libjpeg overview)
    Ready,    ///< GL surface available (see underlay())
    Failed
  };

  /** What the current GL surface represents. */
  enum class Underlay
  {
    None,
    Lqip,    ///< soft placeholder only — grid tiles still required
    Levels   ///< ladder / full overview — gallery may skip_grid
  };

  ImageOverview() :
    m_state(State::Idle),
    m_job(JobHandle::create()),
    m_queue(),
    m_surface(),
    m_underlay(Underlay::None),
    m_levels_requested(false),
    m_lqip_miss_logged(false)
  {}

  State state() const { return m_state; }
  Underlay underlay() const { return m_underlay; }
  bool has_surface() const { return static_cast<bool>(m_surface); }
  /** True when the drawn surface is LQIP only (not levels). */
  bool is_lqip_only() const { return m_underlay == Underlay::Lqip; }

  /** LQIP (and non-thumtoo libjpeg overview). Does not spend tile budget.
   *  @param target_long_edge On-screen long edge (for non-thumtoo DCT scale).
   */
  void ensure_requested(JobManager* job_manager, URL const& url,
                        int original_width, int original_height,
                        TileProviderPtr provider = {},
                        int target_long_edge = 256);

  /** Levels ladder upgrade (thumtoo request_pixels). Call *after* grid
   *  issue_requests so tile jobs get the per-frame budget first.
   *  @param target_long_edge On-screen long edge (clamped 128..512).
   *         At tiny sizes (≤128), LQIP alone is enough — no-op.
   *  One attempt per image until clear(); failures do not re-spend budget.
   */
  void ensure_levels(TileProviderPtr provider, int target_long_edge = 256);

  /** Main thread: promote decoded software surface to a GL texture. */
  void process();

  /** Draw stretched to the image rect (under tiles). */
  void draw(wstdisplay::GraphicsContext& gc, Rectf const& image_rect);

  void clear();

private:
  void receive_software(std::optional<surf::SoftwareSurface> surface,
                        bool from_lqip = false);

  State m_state = State::Idle;
  JobHandle m_job = JobHandle::create();
  ThreadMessageQueue2<std::optional<surf::SoftwareSurface>> m_queue;
  wstdisplay::SurfacePtr m_surface;
  Underlay m_underlay = Underlay::None;
  bool m_levels_requested = false;
  /** One-shot debug: avoid per-frame spam when ensure_lqip is empty. */
  bool m_lqip_miss_logged = false;
};


} // namespace galapix

#endif

/* EOF */
