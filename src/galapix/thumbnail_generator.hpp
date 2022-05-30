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

#ifndef HEADER_GALAPIX_GALAPIX_THUMBNAIL_GENERATOR_HPP
#define HEADER_GALAPIX_GALAPIX_THUMBNAIL_GENERATOR_HPP

#include "database/database.hpp"
#include "job/job_manager.hpp"
#include "server/database_thread.hpp"

namespace galapix {

class Options;

class ThumbnailGenerator
{
public:
  ThumbnailGenerator(Options const& opts);
  ~ThumbnailGenerator();

  void run(std::vector<URL> const& urls, bool generate_all_tiles);

private:
  Database       m_database;
  JobManager     m_job_manager;
  DatabaseThread m_database_thread;

private:
  ThumbnailGenerator(ThumbnailGenerator const&) = delete;
  ThumbnailGenerator& operator=(ThumbnailGenerator const&) = delete;
};

} // namespace galapix

#endif

/* EOF */
