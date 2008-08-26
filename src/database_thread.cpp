/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
*/

#include <iostream>
#include <boost/bind.hpp>
#include <assert.h>
#include "file_database.hpp"
#include "tile_database.hpp"
#include "tile_generator_thread.hpp"
#include "database_thread.hpp"

enum DatabaseMessageType 
{
  DATABASE_ALL_FILES_MESSAGE,
  DATABASE_FILE_MESSAGE,
  DATABASE_TILE_MESSAGE,
  DATABASE_STORE_TILE_MESSAGE,
  DATABASE_THREAD_DONE_MESSAGE
};

class DatabaseMessage
{
public:
  DatabaseMessageType type;

  DatabaseMessage(DatabaseMessageType type_)
    : type(type_)
  {}

  virtual ~DatabaseMessage()
  {}
};

class TileDatabaseMessage : public DatabaseMessage
{
public:
  JobHandle job_handle;

  FileEntry file_entry;
  int tilescale;
  Vector2i pos;
  boost::function<void (TileEntry)> callback;

  TileDatabaseMessage(const JobHandle& job_handle,
                      const FileEntry& file_entry, int tilescale, const Vector2i& pos,
                      const boost::function<void (TileEntry)>& callback)
    : DatabaseMessage(DATABASE_TILE_MESSAGE),
      job_handle(job_handle),
      file_entry(file_entry),
      tilescale(tilescale),
      pos(pos),
      callback(callback)
  {}
};

class FileDatabaseMessage : public DatabaseMessage
{
public:
  std::string filename;
  boost::function<void (FileEntry)> callback;

  FileDatabaseMessage(const std::string& filename,
                      const boost::function<void (FileEntry)>& callback)
    : DatabaseMessage(DATABASE_FILE_MESSAGE),
      filename(filename),
      callback(callback)
  {}
};

class AllFilesDatabaseMessage : public DatabaseMessage
{
public:
  boost::function<void (FileEntry)> callback;

  AllFilesDatabaseMessage(const boost::function<void (FileEntry)>& callback)
    : DatabaseMessage(DATABASE_ALL_FILES_MESSAGE),
      callback(callback)
  {
  }
};

class ThreadDoneDatabaseMessage : public DatabaseMessage
{
public: 
  int threadid;

  ThreadDoneDatabaseMessage(int threadid)
    : DatabaseMessage(DATABASE_THREAD_DONE_MESSAGE),
      threadid(threadid)
  {
  }
};

class StoreTileDatabaseMessage : public DatabaseMessage
{
public:
  TileEntry tile;

  StoreTileDatabaseMessage(const TileEntry& tile)
    : DatabaseMessage(DATABASE_STORE_TILE_MESSAGE),
      tile(tile)
  {}
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
}

JobHandle
DatabaseThread::request_tile(const FileEntry& file_entry, int tilescale, const Vector2i& pos, const boost::function<void (TileEntry)>& callback)
{
  JobHandle job_handle;
  queue.push(new TileDatabaseMessage(job_handle, file_entry, tilescale, pos, callback));
  return job_handle;
}

void
DatabaseThread::request_file(const std::string& filename, const boost::function<void (FileEntry)>& callback)
{
  queue.push(new FileDatabaseMessage(filename, callback));
}

void
DatabaseThread::request_all_files(const boost::function<void (FileEntry)>& callback)
{
  queue.push(new AllFilesDatabaseMessage(callback));
}

void
DatabaseThread::receive_tile(const TileEntry& tile)
{
  queue.push(new StoreTileDatabaseMessage(tile));
}

void
DatabaseThread::receive_job_finished(int threadid)
{
  
}

void
DatabaseThread::stop()
{
  quit = true;
}

int
DatabaseThread::run()
{
  quit = false;

  std::cout << "Connecting to the database..." << std::endl;
  SQLiteConnection db(database_filename);
  FileDatabase file_db(&db);
  TileDatabase tile_db(&db);
  std::cout << "Connecting to the database... done" << std::endl;

  std::vector<DatabaseMessage*> messages;
  while(!quit)
    {
      //std::cout << "DatabaseThread: looping" << std::endl;
      
      // FIXME: Ugly hack to reverse the order of the queue
      while(!queue.empty() && !quit)
        {
          DatabaseMessage* msg = queue.front();
          queue.pop();

          switch(msg->type)
            {
              case DATABASE_STORE_TILE_MESSAGE:
                {
                  StoreTileDatabaseMessage* tile_msg = static_cast<StoreTileDatabaseMessage*>(msg);
                  
                  for(std::list<TileDatabaseMessage*>::iterator i = tile_queue.begin(); i != tile_queue.end();)
                    {
                      if (tile_msg->tile.fileid == (*i)->file_entry.fileid &&
                          tile_msg->tile.scale  == (*i)->tilescale &&
                          tile_msg->tile.pos    == (*i)->pos)
                        {
                          (*i)->callback(tile_msg->tile);

                          delete *i;

                          i = tile_queue.erase(i);
                        }
                      else
                        {
                          ++i;
                        }
                    }

                  tile_db.store_tile(tile_msg->tile);
                }
                break;

              case DATABASE_THREAD_DONE_MESSAGE:
                {
                  //ThreadDoneDatabaseMessage* thread_msg = static_cast<ThreadDoneDatabaseMessage*>(msg);

                  // get thread this message is refering too and mark it
                  // as ready to take new jobs, or join() it to clean up

                  // Also check if new jobs are in the queue to be given to the thread
                }
                break;

              case DATABASE_FILE_MESSAGE:
                {
                  FileDatabaseMessage* file_msg = static_cast<FileDatabaseMessage*>(msg);
                  FileEntry entry;

                  //std::cout << "Lookup for: " << file_msg->filename << std::endl;
                  if (file_db.get_file_entry(file_msg->filename, &entry))
                    {
                      //std::cout << entry.filename << " -> " << entry.fileid << std::endl;
                      file_msg->callback(entry);
                    }
                  else
                    {
                      std::cout << "Error: Couldn't get FileEntry for " << file_msg->filename << std::endl;
                    }
                }
                break;

              case DATABASE_ALL_FILES_MESSAGE:
                {
                  AllFilesDatabaseMessage* all_files_msg = static_cast<AllFilesDatabaseMessage*>(msg);
                  std::vector<FileEntry> entries;
                  file_db.get_file_entries(entries);
                  for(std::vector<FileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
                    {
                      all_files_msg->callback(*i);
                    }
                }
                break;

              case DATABASE_TILE_MESSAGE:
                {
                  TileDatabaseMessage* tile_msg = static_cast<TileDatabaseMessage*>(msg);

                  if (!tile_msg->job_handle.is_aborted())
                    {
                      TileEntry tile;
                      if (tile_db.get_tile(tile_msg->file_entry.fileid, tile_msg->tilescale, tile_msg->pos, tile))
                        {
                          tile_msg->callback(tile);
                          tile_msg->job_handle.finish();
                        }
                      else
                        {
                          if (0)
                            std::cout << "Error: Couldn't get tile: " 
                                      << tile_msg->file_entry.fileid << " "
                                      << tile_msg->pos.x << " "
                                      << tile_msg->pos.y << " "
                                      << tile_msg->tilescale
                                      << std::endl;
                          
                          tile_queue.push_back(tile_msg);

                          //std::cout << tile_queue.size() << std::endl;
                          msg = 0; // FIXME: HACK so that msg doesn't get deleted
                        }
                    }
                }
                break;

              default:
                assert(!"Unknown message type");
            }

          delete msg;
        }
      
      queue.wait();

      std::cout << tile_queue.size() << " vs " << queue.size() << std::endl;

      if (queue.empty() && // FIXME UGLY: to make load on demand somewhat usable
          TileGeneratorThread::current() && !TileGeneratorThread::current()->is_working() && !tile_queue.empty())
        {
          TileGeneratorThread::current()->request_tiles(tile_queue.back()->file_entry,
                                                        tile_queue.back()->tilescale,
                                                        tile_queue.back()->tilescale,
                                                        boost::bind(&DatabaseThread::receive_tile, this, _1));
        }
    }

  return 0;
}

/* EOF */
