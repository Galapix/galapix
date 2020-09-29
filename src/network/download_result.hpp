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

#ifndef HEADER_GALAPIX_NETWORK_DOWNLOAD_RESULT_HPP
#define HEADER_GALAPIX_NETWORK_DOWNLOAD_RESULT_HPP

#include <curl/curl.h>
#include <string>
#include <vector>

class DownloadResult
{
public:
  static DownloadResult from_curl(CURL* handle, std::vector<uint8_t> data);

private:
  DownloadResult() :
    m_content_type(),
    m_data(),
    m_mtime(-1),
    m_response_code()
  {}

public:
  std::string get_content_type() const { return m_content_type; }
  long get_mtime() const { return m_mtime; }
  std::vector<uint8_t>&& move_data() { return std::move(m_data); }
  std::vector<uint8_t> const& get_data() const { return m_data; }
  long get_response_code() const { return m_response_code; }
  bool success() const {
    return m_response_code / 100 == 2;
  }

private:
  std::string m_content_type;
  std::vector<uint8_t> m_data;
  long m_mtime;
  long m_response_code;
};

#endif

/* EOF */
