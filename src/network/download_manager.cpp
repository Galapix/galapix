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

#include "network/download_manager.hpp"

#include <algorithm>
#include <assert.h>
#include <curl/curl.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "network/curl.hpp"
#include "util/raise_exception.hpp"

DownloadManager::DownloadManager() :
  m_thread(),
  m_pipefd(),
  m_abort(false),
  m_stop(false),
  m_queue(),
  m_transfers(),
  m_multi_handle()
{
  int ret = pipe(m_pipefd);
  if (ret < 0)
  {
    raise_runtime_error("pipe() failed: " << strerror(errno));
  }

  if (curl_global_init(CURL_GLOBAL_ALL) != 0)
  {
    raise_runtime_error("curl_global_init() failed");
  }

  m_multi_handle = curl_multi_init();

  m_thread = std::thread(std::bind(&DownloadManager::run, this));
  /*
    When using multiple threads you should set the CURLOPT_NOSIGNAL
    option to 1 for all handles. Everything will or might work fine
    except that timeouts are not honored during the DNS lookup -
    which you can work around by building libcurl with c-ares
    support. c-ares is a library that provides asynchronous name
    resolves. On some platforms, libcurl simply will not function
    properly multi-threaded unless this option is set.
  */
}

DownloadManager::~DownloadManager()
{
  stop();

  try 
  {
    m_thread.join();
  }
  catch(const std::exception& err)
  {
    log_error(err.what());
  }
   
  for(auto transfer : m_transfers)
  {
    curl_multi_remove_handle(m_multi_handle, transfer->handle);
    delete transfer;
  }
  curl_multi_cleanup(m_multi_handle); 
  curl_global_cleanup();

  close(m_pipefd[0]);
  close(m_pipefd[1]);
}

void
DownloadManager::abort()
{
  m_abort = true;
  // send a signal to the wakeup pipe to break out of select
  char buf[1] = {0};
  ssize_t ret = write(m_pipefd[1], buf, sizeof(buf));
  if (ret < 0)
  {
    log_error("write() to pipe failed: " << strerror(errno));
  }
}

void
DownloadManager::stop()
{
  m_stop = true;
  // send a signal to the wakeup pipe to break out of select
  char buf[1] = {0};
  ssize_t ret = write(m_pipefd[1], buf, sizeof(buf));
  if (ret < 0)
  {
    log_error("write() to pipe failed: " << strerror(errno));
  }
}

void
DownloadManager::run()
{
  int running_handles = 0;
  while(!m_abort)
  {
    // if no transfers are in progress, wait for data on the queue
    log_info("transfers: " << m_transfers.size());
    log_info("queue size: " << m_queue.size());
    if (m_transfers.empty())
    {
      log_info("wait for data");
      m_queue.wait_for_data();
      log_info("got data");
    }

    // process everything in the queue
    log_info("try pop");
    std::function<void()> func;
    while(m_queue.try_pop(func))
    {
      log_info("poped");
      func();
    }

    // wait for data to be available to cURL
    if (!m_transfers.empty())
    {
      {
        // wait till data is ready
        fd_set read_fd_set;
        fd_set write_fd_set;
        fd_set exc_fd_set;
        int max_fd;

        FD_ZERO(&read_fd_set);
        FD_ZERO(&write_fd_set);
        FD_ZERO(&exc_fd_set);
      
        curl_multi_fdset(m_multi_handle, &read_fd_set, &write_fd_set, &exc_fd_set, &max_fd);
       
        if (max_fd == -1)
        {
          log_info("XXXXXXXXXX sleep a bit XXXXXXXXXXXXXXX");
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        else
        {
          // add the wakeup pipe into the mix
          FD_SET(m_pipefd[0], &read_fd_set);
          max_fd = std::max(m_pipefd[0], max_fd);

          log_info("select: transfer" << m_transfers.size() << " " << running_handles);
          /*struct timeval timeout;
          timeout.tv_sec  = 1;
          timeout.tv_usec = 0;*/
          select(max_fd+1, &read_fd_set, &write_fd_set, &exc_fd_set, NULL);
          log_info("select done");

          if (FD_ISSET(m_pipefd[0], &read_fd_set))
          {
            // eat up the junk written to the wakeup pipe
            char buf[32];
            read(m_pipefd[0], buf, 32);
          }
        }
      }

      // give control to cURL to do it's thing
      {
        curl_multi_perform(m_multi_handle, &running_handles);
        log_info("-- running handles: " << running_handles);

        int msgs_in_queue;
        CURLMsg* msg;
        while ((msg = curl_multi_info_read(m_multi_handle, &msgs_in_queue)))
        {
          log_info("-- msgs_in_queue: " << msgs_in_queue);

          switch(msg->msg)
          {
            case CURLMSG_DONE:
              log_info("transfer done: " << msg->easy_handle);
              curl_multi_remove_handle(m_multi_handle, msg->easy_handle);
              finish_transfer(msg->easy_handle);
              break;

            default:
              break;
          }
        }
      }
    }

    if (m_stop && m_queue.empty() && running_handles == 0)
    {
      break;
    }
  }
}

void
DownloadManager::finish_transfer(CURL* handle)
{
  auto it = std::find_if(m_transfers.begin(), m_transfers.end(), [handle](Transfer* transfer) {
      return transfer->handle == handle;
    });
  if (it == m_transfers.end())
  {
    log_error("handle not found, this shouldn't happen");
  }
  else
  {
    Transfer* transfer = *it;
    DownloadResult result;

    char* content_type = nullptr;
    curl_easy_getinfo(transfer->handle, CURLINFO_CONTENT_TYPE, &content_type);
    if (content_type)
    {
      result.mime_type = content_type;
    }
    result.blob = Blob::copy(transfer->data);

    if (!transfer->callback)
    {
      log_error("transfer callback not set");
    }
    else
    {
      transfer->callback(result);
    }
    
    m_transfers.erase(it);
    delete *it;
  }
}

size_t
DownloadManager::write_callback_wrap(void* ptr, size_t size, size_t nmemb, void* userdata)
{
  DownloadManager::Transfer* transfer = static_cast<Transfer*>(userdata);
  std::copy((uint8_t*)ptr, (uint8_t*)ptr + size*nmemb, std::back_inserter(transfer->data));
  return nmemb * size;
}

int
DownloadManager::progress_callback_wrap(void* userdata, double dltotal, double dlnow, double ultotal, double ulnow)
{
  DownloadManager::Transfer* transfer = static_cast<Transfer*>(userdata);
  return transfer->progress_callback(dltotal, dlnow);
}

void
DownloadManager::request_url(const std::string& url,
                             const std::function<void (const DownloadResult&)>& callback,
                             const std::function<bool (double total, double now)>& progress_callback)
{
  //JobHandle m_job_manager(std::shared_ptr<Job> job,
  // const std::function<void (std::shared_ptr<Job>, bool)>& callback 
  //                          = std::function<void (std::shared_ptr<Job>, bool)>());

  log_info("request: " << url);
  m_queue.wait_and_push([=]{
      log_info("creating transfer: " << url);
      Transfer* transfer = new Transfer;
     
      transfer->callback = callback;

      curl_easy_setopt(transfer->handle, CURLOPT_WRITEFUNCTION, &DownloadManager::write_callback_wrap);
      curl_easy_setopt(transfer->handle, CURLOPT_WRITEDATA, transfer); // userdata
      curl_easy_setopt(transfer->handle, CURLOPT_URL, url.c_str());
      curl_easy_setopt(transfer->handle, CURLOPT_ERRORBUFFER, transfer->errbuf);

      if (progress_callback)
      {
        transfer->progress_callback = progress_callback;
        curl_easy_setopt(transfer->handle, CURLOPT_NOPROGRESS, 0);
        curl_easy_setopt(transfer->handle, CURLOPT_PROGRESSFUNCTION, &DownloadManager::progress_callback_wrap);
        curl_easy_setopt(transfer->handle, CURLOPT_PROGRESSDATA, transfer);
      }

      // Fake the referer
      curl_easy_setopt(transfer->handle, CURLOPT_REFERER, url.c_str());

      curl_multi_add_handle(m_multi_handle, transfer->handle);

      m_transfers.push_back(transfer);
    });

  // send a signal to the wakeup pipe to break out of select
  char buf[1] = {0};
  ssize_t ret = write(m_pipefd[1], buf, sizeof(buf));
  if (ret < 0)
  {
    log_error("write() to pipe failed: " << strerror(errno));
  }
}

DownloadManager::Transfer::Transfer() :
  handle(curl_easy_init()),
  errbuf(),
  data(),
  callback(),
  progress_callback()
{
}

DownloadManager::Transfer::~Transfer()
{
  curl_easy_cleanup(handle);
}

/* EOF */
