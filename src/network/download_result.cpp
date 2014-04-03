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

#include "network/download_result.hpp"

DownloadResult
DownloadResult::from_curl(CURL* handle, BlobPtr blob)
{
  DownloadResult result;

  result.m_blob = blob;

  char* content_type = nullptr;
  curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &content_type);
  if (content_type)
  {
    result.m_content_type = content_type;
  }

  curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &result.m_response_code);
  curl_easy_getinfo(handle, CURLINFO_FILETIME, &result.m_mtime);

  return result;
}

/* EOF */
