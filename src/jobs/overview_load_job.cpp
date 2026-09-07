// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "jobs/overview_load_job.hpp"

#include <logmich/log.hpp>

#include "jobs/tile_generator.hpp"
#include "math/size.hpp"

namespace galapix {

OverviewLoadJob::OverviewLoadJob(JobHandle const& job_handle,
                                 URL url,
                                 int min_scale,
                                 Callback callback) :
  Job(job_handle),
  m_url(std::move(url)),
  m_min_scale(min_scale),
  m_callback(std::move(callback))
{
}

void
OverviewLoadJob::run()
{
  if (is_aborted()) {
    return;
  }

  try {
    Size original_size;
    surf::SoftwareSurface surface =
      TileGenerator::load_surface(m_url, m_min_scale, &original_size);

    if (is_aborted()) {
      return;
    }

    if (surface.get_width() <= 0 || surface.get_height() <= 0) {
      log_debug("OverviewLoadJob: empty surface for {}", m_url.str());
      m_callback(std::nullopt);
      get_handle().set_failed();
      return;
    }

    m_callback(std::move(surface));
    get_handle().set_finished();
  } catch (std::exception const& err) {
    log_debug("OverviewLoadJob: failed for {}: {}", m_url.str(), err.what());
    m_callback(std::nullopt);
    get_handle().set_failed();
  } catch (...) {
    log_debug("OverviewLoadJob: failed for {}", m_url.str());
    m_callback(std::nullopt);
    get_handle().set_failed();
  }
}

} // namespace galapix

/* EOF */
