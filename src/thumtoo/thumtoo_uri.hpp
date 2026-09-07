// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_THUMTOO_THUMTOO_URI_HPP
#define HEADER_GALAPIX_THUMTOO_THUMTOO_URI_HPP

#include <optional>
#include <string>
#include <string_view>

#include "util/url.hpp"

namespace galapix {

/** Convert a Galapix URL to a thumtoo Location URI.
 *
 *  - Plain file → file:///abs/path
 *  - Archive member (…//rar:m, …//zip:m, …) → file:///abs/archive//archive:m
 *  - PDF page (…//page:N, 1-based) → file:///abs.pdf//page:N
 *  - http(s) → same URL string (thumtoo optional libcurl fetch)
 *  - Other non-file protocols → empty string
 */
std::string thumtoo_uri_from_url(URL const& url);

/** Convert a thumtoo Location URI (or http(s) URL) back to a Galapix URL.
 *
 *  Accepts the same forms produced by thumtoo_uri_from_url and by thumtoo
 *  Client::list_locators_* (file://…, file://…//archive:…, file://…//page:N,
 *  http(s)://…). Content-id URIs (sha256:…) return nullopt — resolve to a
 *  locator first. Empty / unparseable input → nullopt.
 */
std::optional<URL> url_from_thumtoo_uri(std::string_view uri);

} // namespace galapix

#endif

/* EOF */
