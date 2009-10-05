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

//#include <iostream>
//#include <boost/bind.hpp>
//#include <assert.h>
//#include "math/math.hpp"
//#include "url.hpp"
#include "jobs/tile_generation_job.hpp"
#include "job/job_manager.hpp"
#include "database.hpp"
//#include "file_database.hpp"
//#include "tile_database.hpp"
#include "database_thread.hpp"

class DatabaseMessage
{
public:
  DatabaseMessage()
  {}

  virtual ~DatabaseMessage()
  {}

  virtual void run(Database& db) =0;
};

class RequestTileDatabaseMessage : public DatabaseMessage
{
public:
  JobHandle job_handle;

  FileEntry file_entry;
  int tilescale;
  Vector2i pos;
  boost::function<void (TileEntry)> callback;

  RequestTileDatabaseMessage(const JobHandle& job_handle_,
                             const FileEntry& file_entry_, int tilescale_, const Vector2i& pos_,
                             const boost::function<void (TileEntry)>& callback_)
    : job_handle(job_handle_),
      file_entry(file_entry_),
      tilescale(tilescale_),
      pos(pos_),
      callback(callback_)
  {}

  void run(Database& db)
  {
    if (!job_handle.is_aborted())
      {
        TileEntry tile;
        if (db.tiles.get_tile(file_entry.get_fileid(), tilescale, pos, tile))
          {
            // Tile has been found, so return it and finish up
            if (callback)
              callback(tile);
            job_handle.finish();
          }
        else
          {
            // Tile hasn't been found, so we need to generate it
            if (0)
              std::cout << "Error: Couldn't get tile: " 
                        << file_entry.get_fileid() << " "
                        << pos.x << " "
                        << pos.y << " "
                        << tilescale
                        << std::endl;
                          
            DatabaseThread::current()->generate_tile(job_handle, file_entry, tilescale, pos, callback);
          }
      }
  }
};

class RequestFileDatabaseMessage : public DatabaseMessage
{
public:
  JobHandle job_handle;
  URL url;
  boost::function<void (FileEntry)> callback;

  RequestFileDatabaseMessage(const JobHandle& job_handle_,
                             const URL& url_,
                             const boost::function<void (FileEntry)>& callback_)
    : job_handle(job_handle_),
      url(url_),
      callback(callback_)
  {}

  void run(Database& db)
  {
    if (!job_handle.is_aborted())
      {
        FileEntry entry = db.files.get_file_entry(url);
        if (!entry)
          {
            std::cout << "Error: Couldn't get FileEntry for " << url << std::endl;
            job_handle.finish();
          }
        else
          {
            callback(entry);
            job_handle.finish();
          }
      }
  }
};

class AllFilesDatabaseMessage : public DatabaseMessage
{
public:
  boost::function<void (FileEntry)> callback;

  AllFilesDatabaseMessage(const boost::function<void (FileEntry)>& callback_)
    : callback(callback_)
  {
  }

  void run(Database& db)
  {
    std::vector<FileEntry> entries;
    db.files.get_file_entries(entries);
    for(std::vector<FileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
      {
        callback(*i);
      }
  }
};

class FilesByPatternDatabaseMessage : public DatabaseMessage
{
public:
  std::string m_pattern;
  boost::function<void (FileEntry)> m_callback;

  FilesByPatternDatabaseMessage(const boost::function<void (FileEntry)>& callback, const std::string& pattern)
    : m_pattern(pattern),
      m_callback(callback)
  {
  }

  void run(Database& db)
  {
    std::vector<FileEntry> entries;
    db.files.get_file_entries(entries, m_pattern);
    for(std::vector<FileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
      {
        m_callback(*i);
      }               
  }
};

class ReceiveTileDatabaseMessage : public DatabaseMessage
{
public:
  TileEntry tile;

  ReceiveTileDatabaseMessage(const TileEntry& tile_)
    : tile(tile_)
  {}

  void run(Database& db)
  {
    if (0)
      std::cout << "Received Tile: "
                << tile.get_fileid() << " pos: " 
                << tile.get_pos()    << " scale: " 
                << tile.get_scale()  << std::endl;

    DatabaseThread::current()->process_tile(tile);

    // FIXME: Make some better error checking in case of loading failure
    if (tile.get_surface())
      {
        // Avoid doubble store of tiles in the database
        // FIXME: Test the performance of this
        //if (!db.tiles.has_tile(tile.fileid, tile.pos, tile.scale))
        db.tiles.store_tile_in_cache(tile);
      }
    else
      {
        
      }
  }
};

class DeleteFileEntryDatabaseMessage : public DatabaseMessage
{
public:
  int64_t fileid;

  DeleteFileEntryDatabaseMessage(int64_t fileid_)
    : fileid(fileid_)
  {}

  void run(Database& db)
  {
    SQLiteStatement(&db.db)
      .prepare("BEGIN;")
      .execute();
    SQLiteStatement(&db.db)
      .prepare("DELETE FROM files WHERE fileid = ?1;")
      .bind_int64(1, fileid)
      .execute();
    SQLiteStatement(&db.db)
      .prepare("DELETE FROM tiles WHERE fileid = ?1;")
      .bind_int64(1, fileid)
      .execute();
    SQLiteStatement(&db.db)
      .prepare("END;")
      .execute();
  }
};

DatabaseThread* DatabaseThread::current_ = 0;

DatabaseThread::DatabaseThread(const std::string& filename_)
  : database_filename(filename_),
    quit(false)
{
  assert(current_ == 0);
  current_ = this;
}

DatabaseThread::~DatabaseThread()
{
  assert(quit);
}

JobHandle
DatabaseThread::request_tile(const FileEntry& file_entry, int tilescale, const Vector2i& pos, 
                             const boost::function<void (TileEntry)>& callback)
{
  JobHandle job_handle;
  queue.push(new RequestTileDatabaseMessage(job_handle, file_entry, tilescale, pos, callback));
  return job_handle;
}

JobHandle
DatabaseThread::request_file(const URL& url, const boost::function<void (const FileEntry&)>& callback)
{
  JobHandle job_handle;
  queue.push(new RequestFileDatabaseMessage(job_handle, url, callback));
  return job_handle;
}

void
DatabaseThread::request_all_files(const boost::function<void (FileEntry)>& callback)
{
  queue.push(new AllFilesDatabaseMessage(callback));
}

void
DatabaseThread::request_files_by_pattern(const boost::function<void (FileEntry)>& callback, const std::string& pattern)
{
  queue.push(new FilesByPatternDatabaseMessage(callback, pattern));
}

void
DatabaseThread::receive_tile(const TileEntry& tile)
{
  queue.push(new ReceiveTileDatabaseMessage(tile));
}

void
DatabaseThread::delete_file_entry(int64_t fileid)
{
  queue.push(new DeleteFileEntryDatabaseMessage(fileid));
}

void
DatabaseThread::stop_thread()
{
  quit  = true;
  queue.wakeup();
}

void
DatabaseThread::run()
{
  quit = false;

  Database db(database_filename);

  std::vector<DatabaseMessage*> messages;
  while(!quit)
    {
      queue.wait();

      while(!queue.empty())
        {
          DatabaseMessage* msg = queue.front();
          queue.pop();
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
      JobManager::current()->request(job_ptr, boost::function<void (boost::shared_ptr<Job>)>());
      
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
