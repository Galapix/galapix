/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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

#ifndef HEADER_GALAPIX_NETWORK_DOWNLOAD_MANAGER_HPP
#define HEADER_GALAPIX_NETWORK_DOWNLOAD_MANAGER_HPP

#include <boost/optional.hpp>
#include <boost/uuid/uuid.hpp>
#include <curl/curl.h>
#include <thread>
#include <atomic>

#include "job/job_manager.hpp"
#include "job/thread_message_queue2.hpp"
#include "network/download_cache.hpp"
#include "util/blob.hpp"

class DownloadTransfer;
class DownloadResult;

class DownloadManager
{
public:
  typedef bool ProgressFunc(double dltotal, double dlnow, double ultotal, double ulnow);
  typedef unsigned int TransferHandle;

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

public:
  DownloadManager();
  ~DownloadManager();

  TransferHandle request_get(const std::string& url,
                             const std::function<void (const DownloadResult&)>& callback,
                             const std::function<ProgressFunc>& progress_callback
                             = std::function<ProgressFunc>());

  TransferHandle request_post(const std::string& url,
                              const std::string& data,
                              const std::function<void (const DownloadResult&)>& callback,
                              const std::function<ProgressFunc>& progress_callback
                              = std::function<ProgressFunc>());

  void cancel_transfer(TransferHandle handle);
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
  DownloadManager(const DownloadManager&);
  DownloadManager& operator=(const DownloadManager&);
};

#endif

/* EOF */
