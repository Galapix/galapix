// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#ifndef HEADER_GALAPIX_THUMTOO_THUMTOO_URI_HPP
#define HEADER_GALAPIX_THUMTOO_THUMTOO_URI_HPP

#include <string>

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

} // namespace galapix

#endif

/* EOF */
