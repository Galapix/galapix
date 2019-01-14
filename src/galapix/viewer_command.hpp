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

#include "database/database.hpp"
#include "server/database_thread.hpp"
#include "job/job_manager.hpp"

#include "galapix/options.hpp"

class System;

class ViewerCommand
{
public:
  ViewerCommand(System& system, const Options& options);
  ~ViewerCommand();

  void run(const std::vector<URL>& urls);

private:
  System& m_system;
  Options m_opts;

  Database   m_database;
  JobManager m_job_manager;
  DatabaseThread m_database_thread;
  std::vector<std::string> m_patterns;

private:
  ViewerCommand(const ViewerCommand&) = delete;
  ViewerCommand& operator=(const ViewerCommand&) = delete;
};

#endif

/* EOF */
