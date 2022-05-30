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

#ifndef HEADER_GALAPIX_NETWORK_DOWNLOAD_MANAGER_HPP
#define HEADER_GALAPIX_NETWORK_DOWNLOAD_MANAGER_HPP

#include <optional>
#include <curl/curl.h>
#include <thread>
#include <atomic>

#include "job/thread_message_queue2.hpp"
#include "network/download_cache.hpp"

namespace galapix {

struct DownloadProgress;
struct DownloadResult;
class DownloadTransfer;

class DownloadManager
{
public:
  using ProgressFunc = bool (DownloadProgress const&);
  using TransferHandle = unsigned int;

public:
  DownloadManager();
  ~DownloadManager();

  TransferHandle request_get(std::string const& url,
                             const std::function<void (DownloadResult)>& callback,
                             std::function<ProgressFunc> const& progress_callback = {});

  TransferHandle request_post(std::string const& url,
                              std::string const& data,
                              const std::function<void (DownloadResult)>& callback,
                              std::function<ProgressFunc> const& progress_callback = {});

  void cancel_transfer(TransferHandle id);
  void cancel_all_transfers();

private:
  void stop();

  void run();
  void finish_transfer(CURL* handle);

  TransferHandle generate_transfer_handle();

  void wakeup_pipe();
  void wait_for_curl_data();
  void process_curl_data();

private:
  DownloadCache m_cache;
  std::thread m_thread;
  int m_pipefd[2];

  bool m_abort;
  bool m_stop;
  ThreadMessageQueue2<std::function<void ()> > m_queue;

  std::vector<std::unique_ptr<DownloadTransfer> > m_transfers;
  CURLM* m_multi_handle;

  std::atomic<TransferHandle> m_next_transfer_handle;

private:
  DownloadManager(DownloadManager const&);
  DownloadManager& operator=(DownloadManager const&);
};

} // namespace galapix

#endif

/* EOF */
