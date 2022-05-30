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

#ifndef HEADER_GALAPIX_UTIL_URL_HPP
#define HEADER_GALAPIX_UTIL_URL_HPP

#include <string>
#include <fmt/format.h>
#include <fmt/ostream.h>
#include <sstream>

#include "util/blob.hpp"

/** Use Cases:
    - normal system filename
    - filename inside an archive file
    - weburl (unsupported)
 */
class URL
{
  friend bool operator==(URL const& lhs, URL const& rhs);

public:
  static bool is_url(std::string const& url);

  /** Create a URL from a normal filename */
  static URL from_filename(std::string const& filename);

  /** Create a URL object from a URL string as returned from
      get_url() */
  static URL from_string(std::string const& url);

public:
  URL();
  ~URL();

  std::string get_protocol() const;
  std::string get_payload() const;

  /** Get unique representation of this URL */
  std::string str() const;

  /** Get the filename in a form that can be used with the system */
  std::string get_stdio_name() const;
  bool        has_stdio_name() const;

  /** Get the content of the file in the form of a Blob in case it
      doesn't have a normal system filename */
  Blob get_blob(std::string* mime_type = nullptr) const;
  std::vector<uint8_t> get_data(std::string* mime_type = nullptr) const;

  time_t get_mtime() const;
  size_t get_size() const;

  /** Return true when the URL is only accessible via a very slow
      connection */
  bool is_remote() const;

private:
  // URL: {PROTOCOL}://{PAYLOAD}//{PLUGIN}:{PLUGIN_PAYLOAD}
  // Example:
  // http://www.example.com/foobar.jpg
  // file:///www.example.com/foobar.rar//rar:Filename.jpg
  std::string m_protocol;
  std::string m_payload;
  std::string m_plugin;
  std::string m_plugin_payload;
};

std::ostream& operator<<(std::ostream& out, URL const& url);
bool operator<(URL const& lhs, URL const& rhs);
bool operator==(URL const& lhs, URL const& rhs);

#endif

/* EOF */
