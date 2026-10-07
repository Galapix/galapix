// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_GALAPIX_IMAGE_OPENER_HPP
#define HEADER_GALAPIX_GALAPIX_IMAGE_OPENER_HPP

#include <memory>
#include <string>

#include <entt/entity/entity.hpp>

#include "util/url.hpp"

namespace thumtoo {
class Client;
}

namespace galapix {

class JobManager;
class Workspace;

/** Adds images to a Workspace from URLs, picking the tile provider:
    builtin://mandelbrot, Zoomify ImageProperties.xml, or thumtoo for
    everything else. A thumtoo image whose size is not cached yet is added
    with a placeholder and attached by the workspace's SizeProbeSession. */
class ImageOpener final
{
public:
  /** \a thumtoo may be null: images then stay placeholders */
  ImageOpener(JobManager& job_manager, std::shared_ptr<thumtoo::Client> thumtoo);

  /** entt::null when \a url can't be opened */
  entt::entity open(Workspace& workspace, URL const& url) const;

  /** A thumtoo locator whose URI is already known (e.g. from a listing) */
  entt::entity open_thumtoo(Workspace& workspace, URL const& url, std::string const& uri) const;

  /** Start the background size probe for the images opened so far */
  void start_size_probe(Workspace& workspace) const;

private:
  JobManager& m_job_manager;
  std::shared_ptr<thumtoo::Client> m_thumtoo;

public:
  ImageOpener(ImageOpener const&) = delete;
  ImageOpener& operator=(ImageOpener const&) = delete;
};

} // namespace galapix

#endif

/* EOF */
