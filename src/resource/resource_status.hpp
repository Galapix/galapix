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

#ifndef HEADER_GALAPIX_RESOURCE_RESOURCE_STATUS_HPP
#define HEADER_GALAPIX_RESOURCE_RESOURCE_STATUS_HPP

#include <string>
#include <assert.h>

enum class ResourceStatus
{
  /** ResourceStatus has not been set */
  Unknown,

  /** The underlying Blob could not be accessed, this might be
      temporary (directory not mounted, CD not inserted, etc.) */
  AccessError,

  /** No handler could be found for the given resource */
  UnknownHandler,

  /** The Blob could be accessed but the given handler was unable to
      process it, this effect is permanent */
  HandlerError,

  /** The Blob was fully processed and thumbnails and related cache
      information was generated */
  Success,

  /** Blob processing was started, but hasn't finished yet, the
      information in the cache might be incomplete */
  InProgress,

  /** The resource is a JPEG or seekable archive and cache data is
      generated on demand */
  Incremental
};

ResourceStatus ResourceStatus_from_string(const std::string& value);
std::string to_string(const ResourceStatus& value);

#endif

/* EOF */
