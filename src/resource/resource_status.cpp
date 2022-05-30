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

#include "resource/resource_status.hpp"

#include <stdexcept>

namespace galapix {

ResourceStatus ResourceStatus_from_string(std::string const& value)
{
  if (value == "unknown")
  {
    return ResourceStatus::Unknown;
  }
  else if (value == "access_error")
  {
    return ResourceStatus::AccessError;
  }
  else if (value == "handler_error")
  {
    return ResourceStatus::AccessError;
  }
  else if (value == "unknown_handler")
  {
    return ResourceStatus::UnknownHandler;
  }
  else if (value == "handle_error")
  {
    return ResourceStatus::HandlerError;
  }
  else if (value == "success")
  {
    return ResourceStatus::Success;
  }
  else if (value == "in_progress")
  {
    return ResourceStatus::InProgress;
  }
  else if (value == "incremental")
  {
    return ResourceStatus::Incremental;
  }
  else
  {
    throw std::invalid_argument("can't convert " + value + " to ResourceStatus");
  }
}

std::string to_string(ResourceStatus const& value)
{
  switch(value)
  {
    case ResourceStatus::Unknown:
      return "unknown";

    case ResourceStatus::AccessError:
      return "access_error";

    case ResourceStatus::UnknownHandler:
      return "unknown_handler";

    case ResourceStatus::HandlerError:
      return "handle_error";

    case ResourceStatus::Success:
      return "success";

    case ResourceStatus::InProgress:
      return "in_progress";

    case ResourceStatus::Incremental:
      return "incremental";

    default:
      throw std::invalid_argument("invalid ResourceStatus value");
      break;
  }
}

} // namespace galapix

/* EOF */
