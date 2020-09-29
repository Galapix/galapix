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

#include "network/download_transfer.hpp"

DownloadTransfer::DownloadTransfer(DownloadManager::TransferHandle id_,
                                   const std::string& url_,
                                   const std::optional<std::string>& post_data_,
                                   const std::function<void (const DownloadResult&)>& callback_,
                                   const std::function<DownloadManager::ProgressFunc>& progress_callback_) :
  id(id_),
  url(url_),
  handle(curl_easy_init()),
  errbuf(),
  data(),
  post_data(nullptr),
  progress(),
  callback(callback_),
  progress_callback(progress_callback_)
{
  if (!handle)
  {
    throw std::runtime_error("curl_easy_init() failed");
  }
  else
  {
    curl_easy_setopt(handle, CURLOPT_NOSIGNAL, 1);

    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, &DownloadTransfer::write_callback_wrap);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, this); // userdata

    curl_easy_setopt(handle, CURLOPT_NOPROGRESS, 0);
    curl_easy_setopt(handle, CURLOPT_PROGRESSFUNCTION, &DownloadTransfer::progress_callback_wrap);
    curl_easy_setopt(handle, CURLOPT_PROGRESSDATA, this);

    curl_easy_setopt(handle, CURLOPT_ERRORBUFFER, errbuf);

    curl_easy_setopt(handle, CURLOPT_URL, url.c_str());
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(handle, CURLOPT_REFERER, url.c_str());
    curl_easy_setopt(handle, CURLOPT_FILETIME, 1L);

    if (post_data_)
    {
      post_data = curl_easy_escape(handle, post_data_->c_str(), static_cast<int>(post_data_->size()));
      curl_easy_setopt(handle, CURLOPT_POST, 1);
      curl_easy_setopt(handle, CURLOPT_POSTFIELDS, post_data);
      curl_easy_setopt(handle, CURLOPT_POSTFIELDSIZE, -1);
    }
  }
}

DownloadTransfer::~DownloadTransfer()
{
  if (post_data) {
    curl_free(post_data);
  }
  curl_easy_cleanup(handle);
}

size_t
DownloadTransfer::write_callback_wrap(void* ptr, size_t size, size_t nmemb, void* userdata)
{
  DownloadTransfer& transfer = *static_cast<DownloadTransfer*>(userdata);
  transfer.data.insert(transfer.data.end(),
                       static_cast<uint8_t*>(ptr),
                       static_cast<uint8_t*>(ptr) + size * nmemb);
  return nmemb * size;
}

int
DownloadTransfer::progress_callback_wrap(void* userdata, double dltotal, double dlnow, double ultotal, double ulnow)
{
  DownloadTransfer& transfer = *static_cast<DownloadTransfer*>(userdata);

  transfer.progress.dlnow = dlnow;
  transfer.progress.dltotal = dltotal;

  transfer.progress.ulnow = ulnow;
  transfer.progress.ultotal = ultotal;

  if (transfer.progress_callback) {
    return transfer.progress_callback(transfer.progress);
  } else {
    return false;
  }
}

/* EOF */
