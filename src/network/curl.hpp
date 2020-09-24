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

#ifndef HEADER_GALAPIX_NETWORK_CURL_HPP
#define HEADER_GALAPIX_NETWORK_CURL_HPP

#include <string>
#include <vector>

class CURLHandler
{
public:
  /** Download the given \a url and return the result. If mime_type is
      provided, the returned mime_type is stored therein. */
  static std::vector<uint8_t> get_data(const std::string& url, std::string* mime_type = nullptr);
};

#endif

/* EOF */
