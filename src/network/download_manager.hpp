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

#ifndef HEADER_GALAPIX_DOWNLOAD_DOWNLOAD_MANAGER_HPP
#define HEADER_GALAPIX_DOWNLOAD_DOWNLOAD_MANAGER_HPP

#include <thread>
#include <curl/curl.h>

#include "job/job_manager.hpp"
#include "util/blob.hpp"
#include "job/thread_message_queue2.hpp"

class DownloadResult
{
public:
  DownloadResult() :
    mime_type(),
    blob()
  {}

  std::string mime_type;
  BlobPtr blob;
};

class DownloadManager
{
private:
  struct Transfer
  {
    Transfer();
    ~Transfer();

    CURL* handle;
    char errbuf[CURL_ERROR_SIZE];
    std::vector<uint8_t> data;
    std::function<void (const DownloadResult&)> callback;
    std::function<bool (double total, double now)> progress_callback;

  private:
    Transfer(const Transfer&) = delete;
    Transfer& operator=(const Transfer&) = delete;
  };

private:
  std::thread m_thread;
  int m_pipefd[2];

  bool m_abort;
  bool m_stop;
  ThreadMessageQueue2<std::function<void ()> > m_queue;

  std::vector<Transfer*> m_transfers;
  CURLM* m_multi_handle;

public:
  DownloadManager();
  ~DownloadManager();

  void abort();
  void stop();
  void request_url(const std::string& url, 
                   const std::function<void (const DownloadResult&)>& callback,
                   const std::function<bool (double total, double now)>& progress_callback 
                   = std::function<bool (double total, double now)>());

private:
  void run();
  void finish_transfer(CURL* handle);

  static size_t write_callback_wrap(void* ptr, size_t size, size_t nmemb, void* userdata);
  static int progress_callback_wrap(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow);

private:
  DownloadManager(const DownloadManager&);
  DownloadManager& operator=(const DownloadManager&);
};

#endif

/* EOF */
