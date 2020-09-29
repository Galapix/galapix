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

#include "network/curl.hpp"

#include <memory>
#include <sstream>
#include <stdexcept>

#include <curl/curl.h>

static size_t my_curl_write_callback(void* ptr, size_t size, size_t nmemb, void* userdata)
{
  std::vector<uint8_t>* data = static_cast<std::vector<uint8_t>*>(userdata);
  std::copy(static_cast<uint8_t*>(ptr), static_cast<uint8_t*>(ptr) + size*nmemb, std::back_inserter(*data));
  return nmemb * size;
}

std::vector<uint8_t>
CURLHandler::get_data(const std::string& url, std::string* mime_type)
{
  std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> handle{curl_easy_init(), &curl_easy_cleanup};

  std::vector<uint8_t> data;

  char errbuf[CURL_ERROR_SIZE];
  const char* url_str = url.c_str();
  curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, &my_curl_write_callback);
  curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, &data); // userdata
  curl_easy_setopt(handle.get(), CURLOPT_URL, url_str);
  curl_easy_setopt(handle.get(), CURLOPT_ERRORBUFFER, errbuf);

  // Fake the referer
  curl_easy_setopt(handle.get(), CURLOPT_REFERER, url_str);

  CURLcode ret = curl_easy_perform(handle.get());
  long response_code;
  curl_easy_getinfo(handle.get(), CURLINFO_RESPONSE_CODE, &response_code);
  // curl_easy_getinfo(handle.get(), CURLINFO_FILETIME, ...) // mtime

  // get the mime-type
  if (mime_type)
  {
    char* content_type = nullptr;
    curl_easy_getinfo(handle.get(), CURLINFO_CONTENT_TYPE, &content_type);

    if (content_type)
    {
      *mime_type = content_type;
    }
    else
    {
      *mime_type = "";
    }
  }

  curl_easy_cleanup(handle.get());

  if (response_code/100 != 2 && response_code/100 != 3)
  {
    std::ostringstream str;
    str << "CURLHandler::get_data(): HTTP Error: " << response_code;
    throw std::runtime_error(str.str());
  }

  if (ret == 0)
  {
    return data;
  }
  else
  {
    throw std::runtime_error("CURLHandler::get_data(): " + std::string(errbuf));
    return {};
  }
}

/* EOF */
