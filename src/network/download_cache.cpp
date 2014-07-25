/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "network/download_cache.hpp"

#include <logmich/log.hpp>

DownloadCache::DownloadCache(const std::string& cache_directory) :
  m_cache_directory(cache_directory)
{

}

DownloadCache::~DownloadCache()
{

}

boost::optional<DownloadResult>
DownloadCache::get(const std::string& url)
{
  // cache download results in SQLite
  //
  // if (exist(m_cache_directory + hashed(url)))
  // {
  //
  // }
  log_error("not implemented");
  return boost::optional<DownloadResult>();
}

void
DownloadCache::store(const std::string& url, const DownloadResult& result)
{
  log_error("not implemented: %1%", url);
}

/* EOF */
