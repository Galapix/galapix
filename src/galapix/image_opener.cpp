// Galapix - an image viewer for large image collections
// Copyright (C) 2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "galapix/image_opener.hpp"

#include <iostream>

#include <thumtoo/client.hpp>

#include "galapix/mandelbrot_tile_provider.hpp"
#include "galapix/size_probe_session.hpp"
#include "galapix/workspace.hpp"
#include "galapix/zoomify_tile_provider.hpp"
#include "thumtoo/thumtoo_tile_provider.hpp"
#include "thumtoo/thumtoo_uri.hpp"
#include "util/filesystem.hpp"

namespace galapix {

ImageOpener::ImageOpener(JobManager& job_manager, std::shared_ptr<thumtoo::Client> thumtoo) :
  m_job_manager(job_manager),
  m_thumtoo(std::move(thumtoo))
{
}

entt::entity
ImageOpener::open(Workspace& workspace, URL const& url) const
{
  if (url.get_protocol() == "builtin")
  {
    if (url.get_payload() == "mandelbrot") {
      return workspace.add_image(url, std::make_shared<MandelbrotTileProvider>(m_job_manager));
    }

    std::cout << "ImageOpener: unknown builtin:// requested: " << url << " ignoring" << std::endl;
    return entt::null;
  }

  if (Filesystem::has_extension(url.str(), "ImageProperties.xml")) {
    return workspace.add_image(url, ZoomifyTileProvider::create(url, m_job_manager));
  }

  if (!m_thumtoo) {
    return workspace.add_image(url, TileProviderPtr{});
  }

  return open_thumtoo(workspace, url, thumtoo_uri_from_url(url));
}

entt::entity
ImageOpener::open_thumtoo(Workspace& workspace, URL const& url, std::string const& uri) const
{
  if (!m_thumtoo || uri.empty()) {
    return workspace.add_image(url, TileProviderPtr{});
  }

  // Cache-only lookup, never request_size + drain on the GUI thread
  if (auto sz = m_thumtoo->get_size(uri)) {
    if (auto provider = ThumtooTileProvider::create_from_size(m_thumtoo, uri, sz->width, sz->height)) {
      return workspace.add_image(url, provider);
    }
  }

  std::shared_ptr<SizeProbeSession> probe = workspace.size_probe();
  if (!probe) {
    probe = std::make_shared<SizeProbeSession>();
    workspace.set_size_probe(probe);
  }

  entt::entity const image = workspace.add_image(url, TileProviderPtr{});
  m_thumtoo->request_size(uri, [](std::string, thumtoo::SizeReply) {});
  probe->add_pending(image, uri);
  return image;
}

void
ImageOpener::start_size_probe(Workspace& workspace) const
{
  std::shared_ptr<SizeProbeSession> probe = workspace.size_probe();
  if (m_thumtoo && probe && probe->total() > 0) {
    probe->start_drain(m_thumtoo, probe);
  }
}

} // namespace galapix

/* EOF */
