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

#ifndef HEADER_GALAPIX_NETWORK_DOWNLOAD_TRANSFER_HPP
#define HEADER_GALAPIX_NETWORK_DOWNLOAD_TRANSFER_HPP

#include "network/download_manager.hpp"
#include "network/download_progress.hpp"

namespace galapix {

struct DownloadResult;

class DownloadTransfer
{
private:
  static size_t write_callback_wrap(void* ptr, size_t size, size_t nmemb, void* userdata);
  static int progress_callback_wrap(void* userdata, double dltotal, double dlnow, double ultotal, double ulnow);

public:
  DownloadTransfer(DownloadManager::TransferHandle id,
                   std::string const& url,
                   std::optional<std::string> const& post_data,
                   const std::function<void (DownloadResult const&)>& callback,
                   std::function<DownloadManager::ProgressFunc> const& progress_callback);
  ~DownloadTransfer();

public:
  DownloadManager::TransferHandle id;
  std::string url;
  CURL* handle;
  char errbuf[CURL_ERROR_SIZE];
  std::vector<uint8_t> data;
  char* post_data;

  DownloadProgress progress;

  std::function<void (DownloadResult const&)> callback;
  std::function<DownloadManager::ProgressFunc> progress_callback;

private:
  DownloadTransfer(DownloadTransfer const&) = delete;
  DownloadTransfer& operator=(DownloadTransfer const&) = delete;
};

} // namespace galapix

#endif

/* EOF */
