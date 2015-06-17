/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
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

#include "server/database_thread.hpp"

#include <typeinfo>
#include <logmich/log.hpp>

#define GLM_FORCE_RADIANS
#include <glm/gtx/io.hpp>

#include "database/database.hpp"
#include "job/job_manager.hpp"
#include "jobs/file_entry_generation_job.hpp"
#include "jobs/multiple_tile_generation_job.hpp"
#include "jobs/tile_generation_job.hpp"
#include "resource/file_info.hpp"
#include "resource/resource_locator.hpp"
#include "resource/url_info.hpp"

DatabaseThread* DatabaseThread::current_ = 0;

DatabaseThread::DatabaseThread(Database& database,
                               JobManager& tile_job_manager) :
  m_database(database),
  m_tile_job_manager(tile_job_manager),
  m_quit(false),
  m_abort(false),
  m_request_queue(),
  m_receive_queue(256), // FIXME: Make this configurable
  m_tile_generation_jobs()
{
  assert(current_ == 0);
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
DatabaseThread::request_image_info(const ResourceInfo& resource,
                                   const std::function<void (const boost::optional<ImageInfo>&)>& callback)
{
  m_request_queue.wait_and_push([this, resource, callback](){
      boost::optional<ImageInfo> image_info = m_database.get_resources().get_image_info(resource);
      callback(image_info);
    });
}

void
DatabaseThread::request_resource_info(const ResourceLocator& locator, const BlobInfo& blob,
                                      const std::function<void (const boost::optional<ResourceInfo>&)>& callback)
{
  m_request_queue.wait_and_push
    ([this, callback, locator, blob]()
     {
       boost::optional<ResourceInfo> result = m_database.get_resources().get_resource_info(locator, blob);
       callback(result);
     });
}

void
DatabaseThread::store_resource_info(const ResourceInfo& resource_info,
                                    const std::function<void (const Failable<ResourceInfo>&)>& callback)
{
    m_request_queue.wait_and_push
    ([this, resource_info, callback]()
     {
       RowId id = m_database.get_resources().store_resource_info(resource_info);
       callback(ResourceInfo(id, resource_info));
     });
}

void
DatabaseThread::store_image_info(const ImageInfo& image_info,
                                 const std::function<void (const Failable<ImageInfo>&)>& callback)
{
  m_request_queue.wait_and_push
    ([this, image_info, callback]()
     {
      RowId row_id = m_database.get_resources().store_image_info(image_info);
      callback(ImageInfo(row_id, image_info));
     });
}

void
DatabaseThread::request_file_info(const std::string& path,
                                  const std::function<void (const boost::optional<FileInfo>&)>& callback)
{
  m_request_queue.wait_and_push
    ([this, path, callback]()
     {
      boost::optional<FileInfo> file_info = m_database.get_resources().get_file_info(path);
      callback(file_info);
     });
}

void
DatabaseThread::store_file_info(const FileInfo& file_info,
                                const std::function<void (const Failable<FileInfo>&)>& callback)
{
  m_receive_queue.wait_and_push
    ([this, file_info, callback]()
     {
       RowId row_id = m_database.get_resources().store_file_info(file_info);
       callback(FileInfo(row_id, file_info));
     });
}

void
DatabaseThread::request_url_info(const std::string& url, const std::function<void (const boost::optional<URLInfo>&)>& callback)
{
  m_receive_queue.wait_and_push
    ([this, url, callback]()
     {
       boost::optional<URLInfo> url_info = m_database.get_resources().get_url_info(url);
       callback(url_info);
     });
}

void
DatabaseThread::store_url_info(const URLInfo& url_info,
                               const std::function<void (const Failable<URLInfo>&)>& callback)
{
  m_receive_queue.wait_and_push
    ([this, url_info, callback]()
     {
       RowId id = m_database.get_resources().store_url_info(url_info);
       callback(URLInfo(id, url_info));
     });
}


JobHandle
DatabaseThread::request_tile(const OldFileEntry& file_entry, int tilescale, const Vector2i& pos,
                             const std::function<void (Tile)>& callback)
{
  log_info("%1% %2% %3%", file_entry, tilescale, pos);

  JobHandle job_handle_ = JobHandle::create();

  m_request_queue.wait_and_push
    ([this, job_handle_, file_entry, tilescale, pos, callback]()
     {
       JobHandle job_handle = job_handle_;
       if (!job_handle.is_aborted())
       {
         TileEntry tile;
         if (m_database.get_tiles().get_tile(file_entry.get_id(), tilescale, pos, tile))
         {
           // Tile has been found, so return it and finish up
           if (callback)
           {
             callback(tile);
           }
           job_handle.set_finished();
         }
         else
         {
           // Tile hasn't been found, so we need to generate it
           if (0)
             std::cout << "Error: Couldn't get tile: "
                       << file_entry.get_id() << " "
                       << pos.x << " "
                       << pos.y << " "
                       << tilescale
                       << std::endl;

           {
             DatabaseThread::current()->generate_tile(job_handle, file_entry, tilescale, pos, callback);
           }
         }
       }
     });

  return job_handle_;
}

JobHandle
DatabaseThread::request_tiles(const OldFileEntry& file_entry, int min_scale, int max_scale,
                              const std::function<void (Tile)>& callback)
{
  JobHandle job_handle = JobHandle::create();

  m_request_queue.wait_and_push
    ([this, job_handle, file_entry, min_scale, max_scale, callback]
     {
       if (!job_handle.is_aborted())
       {
         generate_tiles(job_handle,
                        file_entry,
                        min_scale, max_scale,
                        callback);
       }
     });

  return job_handle;
}

void
DatabaseThread::request_job_removal(std::shared_ptr<Job> job, bool)
{
  m_request_queue.wait_and_push([this, job](){
      remove_job(job);
    });
}

JobHandle
DatabaseThread::request_file(const URL& url,
                             const std::function<void (OldFileEntry)>& file_callback)
{
  log_info("%1%", url);

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
DatabaseThread::request_all_files(const std::function<void (OldFileEntry)>& callback_)
{
  std::function<void (OldFileEntry)> callback = callback_; // FIXME: internal error workaround

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
DatabaseThread::request_files_by_pattern(const std::function<void (OldFileEntry)>& callback, const std::string& pattern)
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
DatabaseThread::receive_tile(const RowId& fileid, const Tile& tile)
{
  log_info("%s Tile(%s, %s)", fileid, tile.get_scale(), tile.get_pos());

  m_receive_queue.wait_and_push([this, fileid, tile](){
      // FIXME: Make some better error checking in case of loading failure
      if (tile)
      {
        // FIXME: Test the performance of this
        //if (!m_database.get_tiles().has_tile(tile.fileid, tile.pos, tile.scale))
        m_database.get_tiles().store_tile(fileid, tile);
      }
      else
      {

      }
    });
}

void
DatabaseThread::delete_file_entry(const RowId& fileid)
{
  m_request_queue.wait_and_push([this, fileid](){
      m_database.delete_file_entry(fileid);
    });
}

void
DatabaseThread::request_resource_entry(const RowId& blob_id,
                                       const std::function<void (const boost::optional<ResourceEntry>&)>& callback)
{
  m_request_queue.wait_and_push([=](){
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
DatabaseThread::process_queue(ThreadMessageQueue2<std::function<void()>>& queue)
{
  std::function<void()> func;
  while(!m_abort && queue.try_pop(func))
  {
    //std::cout << "DatabaseThread::queue.size(): " << m_queue.size() << " - " << typeid(*msg).name() << std::endl;
    func();
  }
}

void
DatabaseThread::remove_job(std::shared_ptr<Job> job)
{
  for(std::list<std::shared_ptr<TileGenerationJob> >::iterator i = m_tile_generation_jobs.begin();
      i != m_tile_generation_jobs.end(); ++i)
  {
    if (*i == job)
    {
      m_tile_generation_jobs.erase(i);
      break;
    }
  }
}

void
DatabaseThread::generate_tiles(const JobHandle& job_handle, const OldFileEntry& file_entry,
                               int min_scale, int max_scale,
                               const std::function<void (Tile)>& callback)
{
  // FIXME: We are ignoring the callback, but shouldn't so assert in
  // case somebody tries to use one
  assert(false && callback);

  int min_scale_in_db = -1;
  int max_scale_in_db = -1;

  m_database.get_tiles().get_min_max_scale(file_entry.get_id(), min_scale_in_db, max_scale_in_db);

  std::shared_ptr<MultipleTileGenerationJob>
    job_ptr(new MultipleTileGenerationJob(job_handle,
                                          file_entry.get_url(),
                                          min_scale_in_db, max_scale_in_db,
                                          min_scale, max_scale,
                                          [this, file_entry](const Tile& tile){
                                            receive_tile(file_entry.get_id(), tile);
                                          }));

  // Not removing the job from the queue
  m_tile_job_manager.request(job_ptr);
}

void
DatabaseThread::generate_tile(const JobHandle& job_handle,
                              const OldFileEntry& file_entry, int tilescale, const Vector2i& pos,
                              const std::function<void (Tile)>& callback)
{

  std::list<std::shared_ptr<TileGenerationJob> >::iterator it =
    std::find_if(m_tile_generation_jobs.begin(), m_tile_generation_jobs.end(),
                 [&file_entry](const std::shared_ptr<TileGenerationJob>& job){
                   return job->get_url() == file_entry.get_url();
                 });

  if (it != m_tile_generation_jobs.end() &&
      (*it)->request_tile(job_handle, tilescale, pos, callback))
  {
    // all ok
  }
  else
  {
    // job not there or already running, so create a new one
    int min_scale_in_db = -1;
    int max_scale_in_db = -1;

    if (m_database.get_tiles().get_min_max_scale(file_entry.get_id(), min_scale_in_db, max_scale_in_db))
    {
      if (tilescale >= min_scale_in_db &&
          tilescale <= max_scale_in_db)
      {
        // This means we have got a request for tile generation for a
        // tile that should be already in the database, this can
        // happen when the database is incomplete (say after a "kill
        // -9")
        std::cout << "DatabaseThread::generate_tile: Warning request for Tile which should already be in the database:\n"
                  << "  file: " << file_entry << '\n'
                  << "  pos: " << pos << '\n'
                  << "  tilescale: " << tilescale << '\n'
                  << "  min: " << min_scale_in_db  << '\n'
                  << "  max: " << max_scale_in_db << std::endl;

        // We lie and say that no Tile is in the database, so all get
        // regenerated
        min_scale_in_db = -1;
        max_scale_in_db = -1;
      }
    }

    auto job_ptr = std::make_shared<TileGenerationJob>(file_entry, min_scale_in_db, max_scale_in_db);
    job_ptr->sig_tile_callback().connect(std::bind(&DatabaseThread::receive_tile, this, std::placeholders::_1, std::placeholders::_2));

    job_ptr->request_tile(job_handle, tilescale, pos, callback);

    m_tile_job_manager.request(job_ptr, std::bind(&DatabaseThread::request_job_removal, this, std::placeholders::_1, std::placeholders::_2));

    m_tile_generation_jobs.push_front(job_ptr);
  }
}

void
DatabaseThread::generate_file_entry(const JobHandle& job_handle, const URL& url,
                                    const std::function<void (OldFileEntry)>& file_callback)
{
  std::shared_ptr<FileEntryGenerationJob> job_ptr(new FileEntryGenerationJob(job_handle, url));

  if (file_callback)
  {
    job_ptr->sig_file_callback().connect(file_callback);
  }

  job_ptr->sig_file_callback().connect([this, url](const OldFileEntry& file_entry) {
      receive_file(file_entry);
    });

  m_tile_job_manager.request(job_ptr);
  //m_tile_job_manager.request(job_ptr, std::bind(&DatabaseThread::request_job_removal, this, _1, _2));
  //m_tile_generation_jobs.push_front(job_ptr);
}

void
DatabaseThread::store_file_entry(const JobHandle& job_handle_in,
                                 const URL& url, int size, int mtime, OldFileEntry::Handler handler,
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
DatabaseThread::receive_file(const OldFileEntry& file_entry)
{
  m_receive_queue.wait_and_push([this, file_entry]() {
      m_database.get_resources().store_old_file_entry(file_entry.get_url(),
                                              file_entry.get_blob_entry().get_size(),
                                              file_entry.get_mtime(),
                                              file_entry.get_handler());
    });
}

/* EOF */
