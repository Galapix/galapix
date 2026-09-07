// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2026 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

#include "thumtoo/thumtoo_uri.hpp"

#include <thumtoo/archive.hpp>
#include <thumtoo/pdf.hpp>
#include <thumtoo/uri.hpp>

#include <cctype>
#include <filesystem>

namespace galapix {

namespace {

std::filesystem::path absolute_path_from_payload(std::string const& path_str)
{
  std::filesystem::path path(path_str);
  if (!path.is_absolute()) {
    path = std::filesystem::absolute(path);
  }
  return path;
}

} // namespace

std::string
thumtoo_uri_from_url(URL const& url)
{
  std::string const protocol = url.get_protocol();

  // Remote images: pass through for thumtoo (libcurl when THUMTOO_HAVE_CURL).
  if (protocol == "http" || protocol == "https") {
    return url.str();
  }

  if (protocol != "file") {
    return {};
  }

  std::string const full = url.str();
  // file://PAYLOAD or file://PAYLOAD//PLUGIN:MEMBER
  std::string::size_type const proto_end = full.find("://");
  if (proto_end == std::string::npos) {
    return {};
  }
  std::string rest = full.substr(proto_end + 3);

  std::string::size_type const pipe = rest.find("//");
  if (pipe == std::string::npos) {
    return thumtoo::file_uri_from_path(absolute_path_from_payload(rest));
  }

  std::string archive_path = rest.substr(0, pipe);
  std::string plugin_part = rest.substr(pipe + 2); // PLUGIN:MEMBER or page:N
  std::string::size_type const colon = plugin_part.find(':');
  if (colon == std::string::npos) {
    return {};
  }
  std::string plugin = plugin_part.substr(0, colon);
  std::string member = plugin_part.substr(colon + 1);
  for (char& c : plugin) {
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  }

  std::filesystem::path apath = absolute_path_from_payload(archive_path);

  // PDF page: file:///doc.pdf//page:3  (1-based, thumtoo Location form)
  if (plugin == "page") {
    int page = 0;
    for (char c : member) {
      if (c < '0' || c > '9') {
        return {};
      }
      page = page * 10 + (c - '0');
      if (page > 1'000'000) {
        return {};
      }
    }
    if (page < 1) {
      return {};
    }
    return thumtoo::pdf_page_uri(apath, page);
  }

  // Archive member (rar/zip/archive/…): file:///a.zip//zip:inner.jpg
  return thumtoo::archive_uri(apath, member);
}

std::optional<URL>
url_from_thumtoo_uri(std::string_view uri)
{
  if (uri.empty()) {
    return std::nullopt;
  }

  // Content-addressed ids are not openable as images without a locator.
  if (thumtoo::is_content_id_uri(uri)) {
    return std::nullopt;
  }

  // http(s) and file Location URIs share Galapix URL grammar
  // (protocol://payload[//plugin:payload]).
  if (thumtoo::is_http_uri(uri) || uri.starts_with("file://")) {
    // Validate with thumtoo parser so we reject garbage early.
    if (thumtoo::parse_location(uri)) {
      return URL::from_string(std::string(uri));
    }
    return std::nullopt;
  }

  return std::nullopt;
}

} // namespace galapix

/* EOF */
