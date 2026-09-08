// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_UTIL_STATUS_NOTIFY_HPP
#define HEADER_GALAPIX_UTIL_STATUS_NOTIFY_HPP

#include <functional>
#include <string>

namespace galapix {

/** Optional on-screen status/toast hook.
 *
 *  libgalapix must not depend on ImGui. The app layer registers a handler
 *  (ImguiOverlay::notify) at startup; core code calls status_notify().
 *  With no handler installed the call is a no-op.
 */
void set_status_notify_handler(std::function<void(std::string, float)> handler);
void status_notify(std::string message, float duration_seconds = 3.0f);

} // namespace galapix

#endif

/* EOF */
