// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "thumtoo/thumtoo_uri.hpp"

#include <thumtoo/archive.hpp>
#include <thumtoo/uri.hpp>

#include <filesystem>

namespace galapix {

std::string
thumtoo_uri_from_url(URL const& url)
{
  if (url.get_protocol() != "file") {
    return {};
  }

  std::string const full = url.str();
  // file://PAYLOAD or file://PAYLOAD//PLUGIN:MEMBER
  // Find payload and optional plugin section.
  std::string::size_type const proto_end = full.find("://");
  if (proto_end == std::string::npos) {
    return {};
  }
  std::string rest = full.substr(proto_end + 3);

  std::string::size_type const pipe = rest.find("//");
  if (pipe == std::string::npos) {
    // Plain file path (may already be absolute).
    std::filesystem::path path(rest);
    if (!path.is_absolute()) {
      path = std::filesystem::absolute(path);
    }
    return thumtoo::file_uri_from_path(path);
  }

  std::string archive_path = rest.substr(0, pipe);
  std::string plugin_part = rest.substr(pipe + 2); // PLUGIN:MEMBER
  std::string::size_type const colon = plugin_part.find(':');
  if (colon == std::string::npos) {
    return {};
  }
  // std::string plugin = plugin_part.substr(0, colon);
  std::string member = plugin_part.substr(colon + 1);

  std::filesystem::path apath(archive_path);
  if (!apath.is_absolute()) {
    apath = std::filesystem::absolute(apath);
  }
  return thumtoo::archive_uri(apath, member);
}

} // namespace galapix

/* EOF */
