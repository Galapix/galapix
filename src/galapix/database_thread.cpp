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

#include "galapix/database_thread.hpp"

#include <typeinfo>

#include "database/database.hpp"
#include "galapix/database_message.hpp"
#include "job/job_manager.hpp"
#include "jobs/file_entry_generation_job.hpp"
#include "jobs/multiple_tile_generation_job.hpp"
#include "jobs/tile_generation_job.hpp"
#include "util/log.hpp"

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
  assert(m_quit);
}

JobHandle
DatabaseThread::request_tile(const FileEntry& file_entry, int tilescale, const Vector2i& pos, 
                             const std::function<void (Tile)>& callback)
{
  assert(file_entry);
  JobHandle job_handle = JobHandle::create();
  m_request_queue.wait_and_push(new RequestTileDatabaseMessage(job_handle, file_entry, tilescale, pos, callback));
  return job_handle;
}

JobHandle
DatabaseThread::request_tiles(const FileEntry& file_entry, int min_scale, int max_scale,
                              const std::function<void (Tile)>& callback)
{
  JobHandle job_handle = JobHandle::create();
  m_request_queue.wait_and_push(new RequestTilesDatabaseMessage(job_handle, file_entry, min_scale, max_scale, callback));
  return job_handle;
}

void
DatabaseThread::request_job_removal(std::shared_ptr<Job> job, bool)
{
  m_request_queue.wait_and_push(new RequestJobRemovalDatabaseMessage(job));
}

JobHandle
DatabaseThread::request_file(const URL& url, 
                             const std::function<void (FileEntry)>& file_callback,
                             const std::function<void (FileEntry, Tile)>& tile_callback)
{
  JobHandle job_handle = JobHandle::create();
  m_request_queue.wait_and_push(new RequestFileDatabaseMessage(job_handle, url, file_callback, tile_callback));
  return job_handle;
}

void
DatabaseThread::request_all_files(const std::function<void (FileEntry)>& callback)
{
  m_request_queue.wait_and_push(new AllFilesDatabaseMessage(callback));
}

void
DatabaseThread::request_files_by_pattern(const std::function<void (FileEntry)>& callback, const std::string& pattern)
{
  m_request_queue.wait_and_push(new FilesByPatternDatabaseMessage(callback, pattern));
}

void
DatabaseThread::receive_tile(const FileEntry& file_entry, const Tile& tile)
{
  m_receive_queue.wait_and_push(new ReceiveTileDatabaseMessage(file_entry, tile));
}

void
DatabaseThread::delete_file_entry(const FileId& fileid)
{
  m_request_queue.wait_and_push(new DeleteFileEntryDatabaseMessage(fileid));
}

void
DatabaseThread::stop_thread()
{
  m_quit  = true;
  //m_request_queue.wakeup();
  //m_receive_queue.wakeup();
}

void
DatabaseThread::abort_thread()
{
  m_quit  = true;
  m_abort = true;
  //m_request_queue.wakeup();
  //m_receive_queue.wakeup();
}

void
DatabaseThread::run()
{
  m_quit = false;
  
  while(!m_quit)
  {
    //m_queue.wait();

    process_queue(m_receive_queue);
    process_queue(m_request_queue);
    
    usleep(10000); // FIXME: evil busy wait
  }
}

void
DatabaseThread::process_queue(ThreadMessageQueue2<DatabaseMessage*>& queue)
{ 
  DatabaseMessage* msg;
  while(!m_abort && queue.try_pop(msg))
  {
    //std::cout << "DatabaseThread::queue.size(): " << m_queue.size() << " - " << typeid(*msg).name() << std::endl;
    msg->run(m_database);
    delete msg;
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

struct FindByFileEntry
{
  URL m_url;

  FindByFileEntry(URL url) :
    m_url(url)
  {}
  
  bool operator()(std::shared_ptr<TileGenerationJob> job) const
  {
    return job->get_url() == m_url;
  }
};

void
DatabaseThread::generate_tiles(const JobHandle& job_handle, const FileEntry& file_entry,
                               int min_scale, int max_scale,
                               const std::function<void (Tile)>& callback)
{
  // FIXME: We are ignoring the callback, but shouldn't so assert in
  // case somebody tries to use one
  assert(!callback);

  int min_scale_in_db = -1;
  int max_scale_in_db = -1;

  m_database.get_tiles().get_min_max_scale(file_entry, min_scale_in_db, max_scale_in_db);

  std::shared_ptr<MultipleTileGenerationJob> 
    job_ptr(new MultipleTileGenerationJob(job_handle, 
                                          file_entry,
                                          min_scale_in_db, max_scale_in_db,
                                          min_scale, max_scale,
                                          std::bind(&DatabaseThread::receive_tile, this, file_entry, std::placeholders::_1)));

  // Not removing the job from the queue
  m_tile_job_manager.request(job_ptr);
}

void
DatabaseThread::generate_tile(const JobHandle& job_handle,
                              const FileEntry& file_entry, int tilescale, const Vector2i& pos, 
                              const std::function<void (Tile)>& callback)
{ 
  std::list<std::shared_ptr<TileGenerationJob> >::iterator it = 
    std::find_if(m_tile_generation_jobs.begin(), m_tile_generation_jobs.end(), 
                 FindByFileEntry(file_entry.get_url()));

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

    if (m_database.get_tiles().get_min_max_scale(file_entry, min_scale_in_db, max_scale_in_db))
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

    std::shared_ptr<TileGenerationJob> job_ptr(new TileGenerationJob(file_entry, min_scale_in_db, max_scale_in_db));
    job_ptr->sig_tile_callback().connect(std::bind(&DatabaseThread::receive_tile, this, std::placeholders::_1, std::placeholders::_2));

    job_ptr->request_tile(job_handle, tilescale, pos, callback);

    m_tile_job_manager.request(job_ptr, std::bind(&DatabaseThread::request_job_removal, this, std::placeholders::_1, std::placeholders::_2));

    m_tile_generation_jobs.push_front(job_ptr);
  }
}

void
DatabaseThread::generate_file_entry(const JobHandle& job_handle, const URL& url,
                                    const std::function<void (FileEntry)>& file_callback,
                                    const std::function<void (FileEntry, Tile)>& tile_callback)
{
  //log_info << " << url << " " << job_handle << std::endl;
  std::shared_ptr<FileEntryGenerationJob> job_ptr(new FileEntryGenerationJob(job_handle, url));

  if (file_callback)
  {
    job_ptr->sig_file_callback().connect(file_callback);
  }

  if (tile_callback)
  {
    job_ptr->sig_tile_callback().connect(tile_callback);
  }

  job_ptr->sig_file_callback().connect(std::bind(&DatabaseThread::receive_file, this, std::placeholders::_1));
  job_ptr->sig_tile_callback().connect(std::bind(&DatabaseThread::receive_tile, this, std::placeholders::_1, std::placeholders::_2));

  m_tile_job_manager.request(job_ptr);
  //m_tile_job_manager.request(job_ptr, std::bind(&DatabaseThread::request_job_removal, this, _1, _2));
  //m_tile_generation_jobs.push_front(job_ptr);
}

void
DatabaseThread::store_file_entry(const JobHandle& job_handle, 
                                 const URL& url, const Size& size, int format,
                                 const std::function<void (FileEntry)>& callback)
{
  m_receive_queue.wait_and_push(new StoreFileEntryDatabaseMessage(job_handle, url, size, format, callback));
}

void
DatabaseThread::receive_file(const FileEntry& file_entry)
{
  m_receive_queue.wait_and_push(new ReceiveFileEntryDatabaseMessage(file_entry));
}

/* EOF */
