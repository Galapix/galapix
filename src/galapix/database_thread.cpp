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
#include "jobs/tile_generation_job.hpp"
#include "jobs/multiple_tile_generation_job.hpp"
#include "jobs/file_entry_generation_job.hpp"

DatabaseThread* DatabaseThread::current_ = 0;

DatabaseThread::DatabaseThread(const std::string& filename_,
                               JobManager& tile_job_manager,
                               JobManager& file_entry_job_manager)
  : m_tile_job_manager(tile_job_manager),
    m_file_entry_job_manager(file_entry_job_manager),
    database_filename(filename_),  
    m_quit(false),
    m_abort(false),
    m_queue(),
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
                             const boost::function<void (TileEntry)>& callback)
{
  JobHandle job_handle;
  m_queue.push(new RequestTileDatabaseMessage(job_handle, file_entry, tilescale, pos, callback));
  return job_handle;
}

JobHandle
DatabaseThread::request_tiles(const FileEntry& file_entry, int min_scale, int max_scale,
                              const boost::function<void (TileEntry)>& callback)
{
  JobHandle job_handle;
  m_queue.push(new RequestTilesDatabaseMessage(job_handle, file_entry, min_scale, max_scale, callback));
  return job_handle;
}

void
DatabaseThread::request_job_removal(boost::shared_ptr<Job> job, bool)
{
  m_queue.push(new RequestJobRemovalDatabaseMessage(job));
}

JobHandle
DatabaseThread::request_file(const URL& url, const boost::function<void (const FileEntry&)>& callback)
{
  JobHandle job_handle;
  m_queue.push(new RequestFileDatabaseMessage(job_handle, url, callback));
  return job_handle;
}

void
DatabaseThread::request_all_files(const boost::function<void (FileEntry)>& callback)
{
  m_queue.push(new AllFilesDatabaseMessage(callback));
}

void
DatabaseThread::request_files_by_pattern(const boost::function<void (FileEntry)>& callback, const std::string& pattern)
{
  m_queue.push(new FilesByPatternDatabaseMessage(callback, pattern));
}

void
DatabaseThread::receive_tile(const TileEntry& tile)
{
  m_queue.push(new ReceiveTileDatabaseMessage(tile));
}

void
DatabaseThread::delete_file_entry(int64_t fileid)
{
  m_queue.push(new DeleteFileEntryDatabaseMessage(fileid));
}

void
DatabaseThread::stop_thread()
{
  m_quit  = true;
  m_queue.wakeup();
}

void
DatabaseThread::abort_thread()
{
  m_quit  = true;
  m_abort = true;
  m_queue.wakeup();  
}

void
DatabaseThread::run()
{
  m_quit = false;

  m_database.reset(new Database(database_filename));

  while(!m_quit)
  {
    m_queue.wait();

    while(!m_abort && !m_queue.empty())
    {
      DatabaseMessage* msg = m_queue.front();
      m_queue.pop();

      //std::cout << "DatabaseThread::queue.size(): " << m_queue.size() << " - " << typeid(*msg).name() << std::endl;

      msg->run(*m_database);
      delete msg;
    }
  }
}

void
DatabaseThread::remove_job(boost::shared_ptr<Job> job)
{
  for(std::list<boost::shared_ptr<TileGenerationJob> >::iterator i = m_tile_generation_jobs.begin(); 
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
  FileEntry m_file_entry;

  FindByFileEntry(FileEntry file_entry) :
    m_file_entry(file_entry)
  {}
  
  bool operator()(boost::shared_ptr<TileGenerationJob> job) const
  {
    return job->get_file_entry() == m_file_entry;
  }
};

void
DatabaseThread::generate_tiles(const JobHandle& job_handle, const FileEntry& file_entry,
                               int min_scale, int max_scale,
                               const boost::function<void (TileEntry)>& callback)
{
  // FIXME: We are ignoring the callback, but shouldn't so assert in
  // case somebody tries to use one
  assert(!callback);

  int min_scale_in_db = -1;
  int max_scale_in_db = -1;

  m_database->tiles.get_min_max_scale(file_entry, min_scale_in_db, max_scale_in_db);

  boost::shared_ptr<MultipleTileGenerationJob> 
    job_ptr(new MultipleTileGenerationJob(job_handle, 
                                          file_entry,
                                          min_scale_in_db, max_scale_in_db,
                                          min_scale, max_scale,
                                          boost::bind(&DatabaseThread::receive_tile, this, _1)));

  // Not removing the job from the queue
  m_tile_job_manager.request(job_ptr);
}

void
DatabaseThread::generate_tile(const JobHandle& job_handle,
                              const FileEntry& file_entry, int tilescale, const Vector2i& pos, 
                              const boost::function<void (TileEntry)>& callback)
{ 
  std::list<boost::shared_ptr<TileGenerationJob> >::iterator it = 
    std::find_if(m_tile_generation_jobs.begin(), m_tile_generation_jobs.end(), 
                 FindByFileEntry(file_entry));

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

    if (m_database->tiles.get_min_max_scale(file_entry, min_scale_in_db, max_scale_in_db))
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

    boost::shared_ptr<TileGenerationJob> job_ptr(new TileGenerationJob(file_entry, min_scale_in_db, max_scale_in_db,
                                                                       boost::bind(&DatabaseThread::receive_tile, this, _1)));
    job_ptr->request_tile(job_handle, tilescale, pos, callback);

    m_tile_job_manager.request(job_ptr, boost::bind(&DatabaseThread::request_job_removal, this, _1, _2));

    m_tile_generation_jobs.push_front(job_ptr);
  }
}

void
DatabaseThread::generate_file_entry(const JobHandle& job_handle, const URL& url,
                                    const boost::function<void (FileEntry)>& callback)
{
  boost::shared_ptr<Job> job(new FileEntryGenerationJob(job_handle, url, callback));
  m_file_entry_job_manager.request(job,
                                   boost::function<void (boost::shared_ptr<Job>, bool)>());
}

void
DatabaseThread::store_file_entry(const JobHandle& job_handle, 
                                 const URL& url, const Size& size,
                                 const boost::function<void (FileEntry)>& callback)
{
  m_queue.push(new StoreFileEntryDatabaseMessage(job_handle, url, size, callback));
}

/* EOF */
