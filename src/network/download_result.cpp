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

#include "network/download_result.hpp"

DownloadResult
DownloadResult::from_curl(CURL* handle, std::vector<uint8_t> data)
{
  DownloadResult result;

  result.data = std::move(data);

  char* content_type = nullptr;
  curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &content_type);
  if (content_type)
  {
    result.content_type = content_type;
  }

  curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &result.response_code);
  curl_easy_getinfo(handle, CURLINFO_FILETIME, &result.mtime);

  return result;
}

/* EOF */
