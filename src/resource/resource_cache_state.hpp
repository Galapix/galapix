/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_RESOURCE_RESOURCE_CACHE_STATE_HPP
#define HEADER_GALAPIX_RESOURCE_RESOURCE_CACHE_STATE_HPP

#include <vector>
#include <boost/optional.hpp>

#include "resource/resource_info.hpp"
#include "resource/video_info.hpp"
#include "resource/image_info.hpp"
#include "resource/archive_info.hpp"

class ResourceCacheState
{
private:
public:
  ResourceCacheState();

  ResourceInfo get_resource_info() const;
  ImageInfo    get_image_info() const;
  VideoInfo    get_video_info() const;
  ArchiveInfo  get_archive_info() const;

  std::vector<ResourceCacheState> get_children() const;
};

#endif

/* EOF */
