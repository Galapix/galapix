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
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <curl/curl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "network/curl.hpp"
#include "network/download_transfer.hpp"
#include "network/download_result.hpp"
#include "util/raise_exception.hpp"

DownloadManager::DownloadManager() :
  m_cache("/tmp/cachdir"),
  m_thread(),
  m_pipefd(),
  m_abort(false),
  m_stop(false),
  m_queue(),
  m_transfers(),
  m_multi_handle(),
  m_next_transfer_handle(1)
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
  if (!m_multi_handle)
  {
    raise_runtime_error("curl_multi_init() failed");
  }

  m_thread = std::thread(std::bind(&DownloadManager::run, this));
}

DownloadManager::~DownloadManager()
{
  if (!m_abort)
  {
    stop();
  }

  try 
  {
    m_thread.join();
  }
  catch(const std::exception& err)
  {
    log_error(err.what());
  }
   
  for(auto& transfer : m_transfers)
  {
    curl_multi_remove_handle(m_multi_handle, transfer->handle);
  }
  m_transfers.clear();
  curl_multi_cleanup(m_multi_handle); 
  curl_global_cleanup();

  close(m_pipefd[0]);
  close(m_pipefd[1]);
}

void
DownloadManager::wakeup_pipe()
{
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
  wakeup_pipe();
  m_queue.wakeup();
}

void
DownloadManager::wait_for_curl_data()
{
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
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  else
  {
    // add the wakeup pipe into the mix
    FD_SET(m_pipefd[0], &read_fd_set);
    max_fd = std::max(m_pipefd[0], max_fd);

    select(max_fd+1, &read_fd_set, &write_fd_set, &exc_fd_set, NULL);
        
    if (FD_ISSET(m_pipefd[0], &read_fd_set))
    {
      // eat up the junk written to the wakeup pipe
      char buf[32];
      read(m_pipefd[0], buf, 32);
    }
  }
}

void
DownloadManager::process_curl_data()
{  
  // give control to cURL to do it's thing
  int running_handles;
  curl_multi_perform(m_multi_handle, &running_handles);
        
  int msgs_in_queue;
  CURLMsg* msg;
  while ((msg = curl_multi_info_read(m_multi_handle, &msgs_in_queue)))
  {
    switch(msg->msg)
    {
      case CURLMSG_DONE:
        curl_multi_remove_handle(m_multi_handle, msg->easy_handle);
        finish_transfer(msg->easy_handle);
        break;

      default:
        log_error("unhandled cURL message: " << msg->msg);
        break;
    }
  }
}

void
DownloadManager::run()
{
  while(!(m_stop && m_queue.empty() && m_transfers.empty()))
  {
    if (m_transfers.empty())
    {
      m_queue.wait_for_data();
    }

    std::function<void()> func;
    while(m_queue.try_pop(func))
    {     
      func();
    }

    if (!m_transfers.empty())
    {
      wait_for_curl_data();
      process_curl_data();
    }
  }
}

void
DownloadManager::cancel_transfer(TransferHandle id)
{
  m_queue.wait_and_push(
    [=]{
      auto it = std::find_if(m_transfers.begin(), m_transfers.end(), 
                             [&id](std::unique_ptr<DownloadTransfer>& transfer) {
                               return transfer->id == id;
                             });
      if (it == m_transfers.end())
      {
        log_info("transfer not found: " << id);
      }
      else
      {
        curl_multi_remove_handle(m_multi_handle, (*it)->handle);
        m_transfers.erase(it);
      }
    });
}

void
DownloadManager::cancel_all_transfers()
{
  m_queue.wait_and_push(
    [=]{
      for(auto& transfer : m_transfers)
      {
        curl_multi_remove_handle(m_multi_handle, transfer->handle);
      }
      m_transfers.clear();
    });
}

void
DownloadManager::finish_transfer(CURL* handle)
{
  auto it = std::find_if(m_transfers.begin(), m_transfers.end(), 
                         [handle](std::unique_ptr<DownloadTransfer>& transfer) {
                           return transfer->handle == handle;
                         });
  if (it == m_transfers.end())
  {
    log_error("handle not found, this shouldn't happen");
  }
  else
  {
    auto& transfer = *it;

    if (!transfer->callback)
    {
      log_error("transfer callback not set");
    }
    else
    {
      DownloadResult result = DownloadResult::from_curl(transfer->handle, Blob::copy(transfer->data));
      m_cache.store(transfer->url, result);
      transfer->callback(result);
    }
    
    m_transfers.erase(it);
  }
}

DownloadManager::TransferHandle
DownloadManager::generate_transfer_handle()
{
  return m_next_transfer_handle.fetch_add(1);
}

DownloadManager::TransferHandle
DownloadManager::request_get(const std::string& url,
                             const std::function<void (const DownloadResult&)>& callback,
                             const std::function<ProgressFunc>& progress_callback)
{
  TransferHandle uuid = generate_transfer_handle();

  m_queue.wait_and_push(
    [=]{
      boost::optional<DownloadResult> cached_result = m_cache.get(url);
      if (cached_result)
      {
        log_info("downloading from cache: " << url);
        callback(*cached_result);
      }
      else
      {
        log_info("downloading: " << url);
        std::unique_ptr<DownloadTransfer> transfer(new DownloadTransfer(uuid, url, boost::optional<std::string>(),
                                                                        callback, progress_callback));
        curl_multi_add_handle(m_multi_handle, transfer->handle);
        m_transfers.push_back(std::move(transfer));
      }
    });

  wakeup_pipe();

  return uuid;
}

DownloadManager::TransferHandle
DownloadManager::request_post(const std::string& url,
                              const std::string& post_data,
                              const std::function<void (const DownloadResult&)>& callback,
                              const std::function<ProgressFunc>& progress_callback)
{
  TransferHandle uuid = generate_transfer_handle();

  m_queue.wait_and_push(
    [=]{
      log_info("downloading: " << url);
      std::unique_ptr<DownloadTransfer> transfer(new DownloadTransfer(uuid, url, post_data,
                                                                      callback, progress_callback));
      curl_multi_add_handle(m_multi_handle, transfer->handle);
      m_transfers.push_back(std::move(transfer));
    });

  wakeup_pipe();

  return uuid;
}

/* EOF */
