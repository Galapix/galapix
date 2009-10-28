/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_UTIL_URL_HPP
#define HEADER_GALAPIX_UTIL_URL_HPP

#include <string>

#include "util/blob.hpp"

/** Use Cases: 
    - normal system filename
    - filename inside an archive file
    - weburl (unsupported)
 */
class URL
{
private:
  // URL: {PROTOCOL}://{PAYLOAD}//{PLUGIN}:{PLUGIN_PAYLOAD}
  // Example:
  // http://www.example.com/foobar.jpg
  // file:///www.example.com/foobar.rar//rar:Filename.jpg
  std::string m_protocol;
  std::string m_payload;
  std::string m_plugin;
  std::string m_plugin_payload;

public:
  URL();
  ~URL();

  /** Create a URL from a normal filename */
  static URL from_filename(const std::string& filename);

  /** Create a URL object from a URL string as returned from
      get_url() */
  static URL from_string(const std::string& str);

  std::string get_protocol() const;
  std::string get_payload() const;

  /** Get unique representation of this URL */
  std::string str() const;

  /** Get the filename in a form that can be used with the system */
  std::string get_stdio_name() const;
  bool        has_stdio_name() const;

  /** Get the content of the file in the form of a Blob in case it
      doesn't have a normal system filename */
  BlobPtr  get_blob() const;

  int         get_mtime() const;
  int         get_size() const;

  static bool is_url(const std::string& url);
  friend bool operator==(const URL& lhs, const URL& rhs);  
};

std::ostream& operator<<(std::ostream& out, const URL& url);
bool operator<(const URL& lhs, const URL& rhs);
bool operator==(const URL& lhs, const URL& rhs);

#endif

/* EOF */
