// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "util/status_notify.hpp"

namespace galapix {

namespace {

std::function<void(std::string, float)>& handler_slot()
{
  static std::function<void(std::string, float)> slot;
  return slot;
}

} // namespace

void
set_status_notify_handler(std::function<void(std::string, float)> handler)
{
  handler_slot() = std::move(handler);
}

void
status_notify(std::string message, float duration_seconds)
{
  if (auto const& h = handler_slot()) {
    h(std::move(message), duration_seconds);
  }
}

} // namespace galapix

/* EOF */
