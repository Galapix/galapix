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

#ifndef HEADER_GALAPIX_NETWORK_DOWNLOAD_CACHE_HPP
#define HEADER_GALAPIX_NETWORK_DOWNLOAD_CACHE_HPP

#include <boost/optional.hpp>
#include <string>

#include "network/download_result.hpp"

class DownloadCache
{
private:
  std::string m_cache_directory;

public:
  DownloadCache(const std::string& cache_directory);
  ~DownloadCache();

  boost::optional<DownloadResult> get(const std::string& url);
  void store(const std::string& url, const DownloadResult& result);

private:
  DownloadCache(const DownloadCache&);
  DownloadCache& operator=(const DownloadCache&);
};

#endif

/* EOF */
