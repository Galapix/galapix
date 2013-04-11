/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_NETWORK_DOWNLOAD_RESULT_HPP
#define HEADER_GALAPIX_NETWORK_DOWNLOAD_RESULT_HPP

#include <curl/curl.h>
#include <string>

#include "util/blob.hpp"

class DownloadResult
{
public:
  static DownloadResult from_curl(CURL* handle, BlobPtr blob);

private:
  DownloadResult() :
    m_content_type(),
    m_blob(),
    m_response_code()
  {}

public:
  std::string get_content_type() const { return m_content_type; }
  BlobPtr get_blob() const { return m_blob; }
  long get_response_code() const { return m_response_code; }
  bool success() const {
    return m_response_code / 100 == 2;
  }

private:
  std::string m_content_type;
  BlobPtr m_blob;
  long m_response_code;
};

#endif

/* EOF */
