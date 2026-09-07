// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_GALAPIX_VIEWER_COMMAND_HPP
#define HEADER_GALAPIX_GALAPIX_VIEWER_COMMAND_HPP

#include "job/job_manager.hpp"

#include "galapix/options.hpp"

#include <memory>
#include <string>

#include "galapix/tile_provider.hpp"

namespace thumtoo { class Client; }

namespace galapix {

class System;
class URL;

class ViewerCommand
{
public:
  ViewerCommand(System& system, Options const& opts);
  ~ViewerCommand();

  void run(std::vector<URL> const& urls);

private:
  System& m_system;
  Options m_opts;

  JobManager m_job_manager;
  std::vector<std::string> m_patterns;

#ifdef HAVE_THUMTOO
  std::shared_ptr<thumtoo::Client> m_thumtoo;
#endif

  /** Thumtoo TileProvider when HAVE_THUMTOO; else empty for plain files. */
  TileProviderPtr make_file_tile_provider(URL const& url);

private:
  ViewerCommand(ViewerCommand const&) = delete;
  ViewerCommand& operator=(ViewerCommand const&) = delete;
};

} // namespace galapix

#endif

/* EOF */
