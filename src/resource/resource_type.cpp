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

#include "resource/resource_type.hpp"

#include <stdexcept>

std::ostream& operator<<(std::ostream& os, ResourceType type)
{
  switch(type)
  {
    case ResourceType::Blob:
      return os << "blob";

    case ResourceType::Image:
      return os << "image";

    case ResourceType::Video:
      return os << "video";

    case ResourceType::Archive:
      return os << "archive";

    case ResourceType::Collection:
      return os << "collection";

    default:
      return os << "unknown";
  }
}

ResourceType ResourceType_from_string(std::string const& value)
{
  if (value == "blob")
  {
    return ResourceType::Blob;
  }
  else if (value == "video")
  {
    return ResourceType::Video;
  }
  else if (value == "image")
  {
    return ResourceType::Image;
  }
  else if (value == "archive")
  {
    return ResourceType::Archive;
  }
  else if (value == "collection")
  {
    return ResourceType::Collection;
  }
  else
  {
    throw std::invalid_argument("can't convert '" + value + "' to ResourceType");
  }
}

/* EOF */
