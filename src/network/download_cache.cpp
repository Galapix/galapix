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

#include "network/download_cache.hpp"

#include <logmich/log.hpp>

DownloadCache::DownloadCache(std::string const& cache_directory) :
  m_cache_directory(cache_directory)
{

}

DownloadCache::~DownloadCache()
{

}

std::optional<DownloadResult>
DownloadCache::get(std::string const& url)
{
  // cache download results in SQLite
  //
  // if (exist(m_cache_directory + hashed(url)))
  // {
  //
  // }
  log_error("not implemented");
  return std::optional<DownloadResult>();
}

void
DownloadCache::store(std::string const& url, DownloadResult const& result)
{
  log_error("not implemented: {}", url);
}

/* EOF */
