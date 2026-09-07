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

#include "server/database_thread.hpp"

#include <typeinfo>
#include <logmich/log.hpp>
#include <unistd.h>

#include <glm/gtx/io.hpp>

#include "database/database.hpp"
#include "job/job_manager.hpp"
#include "jobs/file_entry_generation_job.hpp"
#include "resource/file_info.hpp"
#include "resource/resource_locator.hpp"
#include "resource/url_info.hpp"

namespace galapix {

DatabaseThread* DatabaseThread::current_ = nullptr;

DatabaseThread::DatabaseThread(Database& database,
                               JobManager& tile_job_manager) :
  m_database(database),
  m_tile_job_manager(tile_job_manager),
  m_quit(false),
  m_abort(false),
  m_request_queue(),
  m_receive_queue(256) // FIXME: Make this configurable
{
  assert(current_ == nullptr);
  current_ = this;
}

DatabaseThread::~DatabaseThread()
{
  if (!m_quit)
  {
    log_error("m_quit not set while executing ~DatabaseThread()");
  }
}

void
DatabaseThread::stop_thread()
{
  m_quit  = true;
  m_request_queue.wakeup();
  m_receive_queue.wakeup();
}

void
DatabaseThread::abort_thread()
{
  m_quit  = true;
  m_abort = true;
  m_request_queue.wakeup();
  m_receive_queue.wakeup();
}

void
DatabaseThread::request_image_info(ResourceInfo const& resource,
                                   const std::function<void (std::optional<ImageInfo> const&)>& callback)
{
  m_request_queue.wait_and_push([this, resource, callback](){
      std::optional<ImageInfo> image_info = m_database.get_resources().get_image_info(resource);
      callback(image_info);
    });
}

void
DatabaseThread::request_resource_info(ResourceLocator const& locator, BlobInfo const& blob,
                                      const std::function<void (std::optional<ResourceInfo> const&)>& callback)
{
  m_request_queue.wait_and_push
    ([this, callback, locator, blob]()
     {
       std::optional<ResourceInfo> result = m_database.get_resources().get_resource_info(locator, blob);
       callback(result);
     });
}

void
DatabaseThread::store_resource_info(ResourceInfo const& resource_info,
                                    const std::function<void (Failable<ResourceInfo> const&)>& callback)
{
    m_request_queue.wait_and_push
    ([this, resource_info, callback]()
     {
       RowId id = m_database.get_resources().store_resource_info(resource_info);
       callback(ResourceInfo(id, resource_info));
     });
}

void
DatabaseThread::store_image_info(ImageInfo const& image_info,
                                 const std::function<void (Failable<ImageInfo> const&)>& callback)
{
  m_request_queue.wait_and_push
    ([this, image_info, callback]()
     {
      RowId row_id = m_database.get_resources().store_image_info(image_info);
      callback(ImageInfo(row_id, image_info));
     });
}

void
DatabaseThread::request_file_info(std::string const& path,
                                  const std::function<void (std::optional<FileInfo> const&)>& callback)
{
  m_request_queue.wait_and_push
    ([this, path, callback]()
     {
      std::optional<FileInfo> file_info = m_database.get_resources().get_file_info(path);
      callback(file_info);
     });
}

void
DatabaseThread::store_file_info(FileInfo const& file_info,
                                const std::function<void (Failable<FileInfo> const&)>& callback)
{
  m_receive_queue.wait_and_push
    ([this, file_info, callback]()
     {
       RowId row_id = m_database.get_resources().store_file_info(file_info);
       callback(FileInfo(row_id, file_info));
     });
}

void
DatabaseThread::request_url_info(std::string const& url, const std::function<void (std::optional<URLInfo> const&)>& callback)
{
  m_receive_queue.wait_and_push
    ([this, url, callback]()
     {
       std::optional<URLInfo> url_info = m_database.get_resources().get_url_info(url);
       callback(url_info);
     });
}

void
DatabaseThread::store_url_info(URLInfo const& url_info,
                               const std::function<void (Failable<URLInfo> const&)>& callback)
{
  m_receive_queue.wait_and_push
    ([this, url_info, callback]()
     {
       RowId id = m_database.get_resources().store_url_info(url_info);
       callback(URLInfo(id, url_info));
     });
}




void
DatabaseThread::request_job_removal(std::shared_ptr<Job> const& job, bool unused)
{
  (void)job;
  (void)unused;
  // TileGenerationJob queue removed; file-entry jobs are fire-and-forget.
}

JobHandle
DatabaseThread::request_file(URL const& url,
                             const std::function<void (OldFileEntry)>& file_callback)
{
  log_info("{}", url);

  JobHandle job_handle_ = JobHandle::create();

  m_request_queue.wait_and_push
    ([this, job_handle_, url, file_callback]()
     {
      JobHandle job_handle = job_handle_;
      if (!job_handle.is_aborted())
      {
        OldFileEntry file_entry;
        if (!m_database.get_resources().get_old_file_entry(url, file_entry))
        {
          // file entry is not in the database, so try to generate it
          DatabaseThread::current()->generate_file_entry(job_handle, url,
                                                         file_callback);
        }
        else
        {
          file_callback(file_entry);
          job_handle.set_finished();
        }
      }
    });

  return job_handle_;
}

void
DatabaseThread::request_all_files(const std::function<void (OldFileEntry)>& callback)
{
  m_request_queue.wait_and_push([this, callback]{
      std::vector<OldFileEntry> entries;
      m_database.get_resources().get_old_file_entries(entries);
      for(std::vector<OldFileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
      {
        callback(*i);
      }
    });
}

void
DatabaseThread::request_files_by_pattern(const std::function<void (OldFileEntry)>& callback, std::string const& pattern)
{
  log_info(pattern);

  m_request_queue.wait_and_push([this, callback, pattern](){
      std::vector<OldFileEntry> entries;
      m_database.get_resources().get_old_file_entries(pattern, entries);
      for(std::vector<OldFileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
      {
        callback(*i);
      }
      });
}


void
DatabaseThread::delete_file_entry(RowId const& fileid)
{
  m_request_queue.wait_and_push([this, fileid](){
      m_database.delete_file_entry(fileid);
    });
}

void
DatabaseThread::request_resource_entry(RowId const& blob_id,
                                       const std::function<void (std::optional<ResourceEntry> const&)>& callback)
{
  m_request_queue.wait_and_push([=, this](){
      callback(m_database.get_resources().get_resource_entry(blob_id));
    });
}

void
DatabaseThread::run()
{
  m_quit = false;

  while(!m_quit)
  {
    // FIXME: This really should be a priority queue
    process_queue(m_receive_queue);
    process_queue(m_request_queue);

    usleep(10000); // FIXME: evil busy wait
  }
}

void
DatabaseThread::process_queue(ThreadMessageQueue2<std::function<void()>>& queue) const
{
  std::function<void()> func;
  while(!m_abort && queue.try_pop(func))
  {
    //std::cout << "DatabaseThread::queue.size(): " << m_queue.size() << " - " << typeid(*msg).name() << std::endl;
    func();
  }
}




void
DatabaseThread::generate_file_entry(JobHandle const& job_handle, URL const& url,
                                    const std::function<void (OldFileEntry)>& file_callback)
{
  std::shared_ptr<FileEntryGenerationJob> job_ptr(new FileEntryGenerationJob(job_handle, url));

  if (file_callback)
  {
    job_ptr->sig_file_callback().connect(file_callback);
  }

  job_ptr->sig_file_callback().connect([this, url](OldFileEntry const& file_entry) {
      receive_file(file_entry);
    });

  m_tile_job_manager.request(job_ptr);
}

void
DatabaseThread::store_file_entry(JobHandle const& job_handle_in,
                                 URL const& url, int size, int mtime, OldFileEntry::Handler handler,
                                 const std::function<void (OldFileEntry)>& callback)
{
  m_receive_queue.wait_and_push([this, job_handle_in, url, size, mtime, handler, callback](){
      JobHandle job_handle = job_handle_in;
      OldFileEntry file_entry = m_database.get_resources().store_old_file_entry(url, size, mtime, handler);
      if (callback)
      {
        callback(file_entry);
      }
      job_handle.set_finished();
    });
}

void
DatabaseThread::receive_file(OldFileEntry const& file_entry)
{
  m_receive_queue.wait_and_push([this, file_entry]() {
      m_database.get_resources().store_old_file_entry(file_entry.get_url(),
                                              file_entry.get_blob_entry().get_size(),
                                              file_entry.get_mtime(),
                                              file_entry.get_handler());
    });
}

} // namespace galapix

/* EOF */
