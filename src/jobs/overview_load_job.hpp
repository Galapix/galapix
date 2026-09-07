// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_JOBS_OVERVIEW_LOAD_JOB_HPP
#define HEADER_GALAPIX_JOBS_OVERVIEW_LOAD_JOB_HPP

#include <functional>
#include <optional>

#include <surf/software_surface.hpp>

#include "job/job.hpp"
#include "util/url.hpp"

namespace galapix {

/** Load a low-quality whole-image preview off the GUI thread.
 *
 *  Uses TileGenerator::load_surface (libjpeg DCT scale for JPEG). Result is
 *  NOT a grid tile and must not be written into the tile cache.
 */
class OverviewLoadJob final : public Job
{
public:
  using Callback = std::function<void(std::optional<surf::SoftwareSurface>)>;

  OverviewLoadJob(JobHandle const& job_handle,
                  URL url,
                  int min_scale,
                  Callback callback);

  void run() override;

private:
  URL m_url;
  int m_min_scale;
  Callback m_callback;
};

} // namespace galapix

#endif

/* EOF */
