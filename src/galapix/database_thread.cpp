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
#include "jobs/file_entry_generation_job.hpp"

DatabaseThread* DatabaseThread::current_ = 0;

DatabaseThread::DatabaseThread(const std::string& filename_,
                               JobManager& tile_job_manager,
                               JobManager& file_entry_job_manager)
  : m_tile_job_manager(tile_job_manager),
    m_file_entry_job_manager(file_entry_job_manager),
    database_filename(filename_),
    abort_instantly(),
    m_quit(false),
    m_abort(false),
    m_queue(),
    tile_request_groups()
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

  Database db(database_filename);

  while(!m_quit)
  {
    m_queue.wait();

    while(!m_abort && !m_queue.empty())
    {
      DatabaseMessage* msg = m_queue.front();
      m_queue.pop();

      //std::cout << "DatabaseThread::queue.size(): " << m_queue.size() << " - " << typeid(*msg).name() << std::endl;

      msg->run(db);
      delete msg;
    }
  }
}

void
DatabaseThread::generate_tile(const JobHandle& job_handle,
                              const FileEntry& file_entry, int tilescale, const Vector2i& pos, 
                              const boost::function<void (TileEntry)>& callback)
{
  // FIXME: We don't check what tiles are already present, so some get
  // generated multiple times! (3-5 then 0-5, while it should be 0-2)
  // FIXME: Cancelation doesn't work either

  TileRequestGroups::iterator group = tile_request_groups.end();
  for(TileRequestGroups::iterator i = tile_request_groups.begin(); i != tile_request_groups.end(); ++i)
    {
      if (i->fileid    == file_entry.get_fileid() && 
          i->min_scale <= tilescale &&
          i->max_scale >= tilescale)
        {
          group = i;
          break;
        }
    }

  if (group == tile_request_groups.end())
    {
      int min_scale = tilescale;
      int max_scale = file_entry.get_thumbnail_scale();
      boost::shared_ptr<Job> job_ptr(new TileGenerationJob(job_handle, 
                                                           file_entry, min_scale, max_scale, 
                                                           boost::bind(&DatabaseThread::receive_tile, this, _1)));
      m_tile_job_manager.request(job_ptr, boost::function<void (boost::shared_ptr<Job>)>());
      
      TileRequestGroup request_group(file_entry.get_fileid(), min_scale, max_scale);
      request_group.requests.push_back(TileRequest(job_handle, tilescale, pos, callback));
      tile_request_groups.push_back(request_group);
    }
  else
    {
      group->requests.push_back(TileRequest(job_handle, tilescale, pos, callback));
    }
}

void
DatabaseThread::generate_file_entry(const JobHandle& job_handle, const URL& url,
                                    const boost::function<void (FileEntry)>& callback)
{
  boost::shared_ptr<Job> job(new FileEntryGenerationJob(job_handle, url, callback));
  m_file_entry_job_manager.request(job,
                                   boost::function<void (boost::shared_ptr<Job>)>());
}

void
DatabaseThread::store_file_entry(const URL& url, const Size& size,
                                 const boost::function<void (FileEntry)>& callback)
{
  m_queue.push(new StoreFileEntryDatabaseMessage(url, size, callback));
}

void
DatabaseThread::process_tile(const TileEntry& tile_entry)
{
  // FIXME: Could break/return here to do some less looping
  for(TileRequestGroups::iterator i = tile_request_groups.begin(); i != tile_request_groups.end(); ++i)
    {
      if (i->fileid == tile_entry.get_fileid())
        {
          for(std::vector<TileRequest>::iterator j = i->requests.begin(); j != i->requests.end(); ++j)
            {
              if (tile_entry.get_scale() == j->scale &&
                  tile_entry.get_pos()   == j->pos)
                {
                  if (j->callback)
                    j->callback(tile_entry);

                  j->job_handle.finish();
                  i->requests.erase(j);
                  break;
                }
            }
        }
    }
}

/* EOF */
