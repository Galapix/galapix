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
  DATABASE_STORE_TILE_MESSAGE
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

  int fileid;
  int tilescale;
  int x;
  int y;
  boost::function<void (TileEntry)> callback;

  TileDatabaseMessage(const JobHandle& job_handle,
                      int fileid, int tilescale, int x, int y,
                      const boost::function<void (TileEntry)>& callback)
    : DatabaseMessage(DATABASE_TILE_MESSAGE),
      job_handle(job_handle),
      fileid(fileid),
      tilescale(tilescale),
      x(x),
      y(y),
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
DatabaseThread::request_tile(int fileid, int tilescale, int x, int y, const boost::function<void (TileEntry)>& callback)
{
  JobHandle job_handle;
  queue.push(new TileDatabaseMessage(job_handle, fileid, tilescale, x, y, callback));
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
DatabaseThread::store_tile(const TileEntry& tile)
{
  queue.push(new StoreTileDatabaseMessage(tile));
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
      
      // do things
      while(!queue.empty() && !quit)
        {
          messages.push_back(queue.front());
          queue.pop();
        }

      if (!messages.empty())
        {
          DatabaseMessage* msg = messages.back();
          messages.pop_back();

          switch(msg->type)
            {
              case DATABASE_STORE_TILE_MESSAGE:
                {
                  StoreTileDatabaseMessage* tile_msg = static_cast<StoreTileDatabaseMessage*>(msg);
                  tile_db.store_tile(tile_msg->tile);
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
                      if (tile_db.get_tile(tile_msg->fileid, tile_msg->tilescale, tile_msg->x, tile_msg->y, tile))
                        {
                          tile_msg->callback(tile);
                          tile_msg->job_handle.finish();
                        }
                      else
                        {
                          if (0)
                            std::cout << "Error: Couldn't get tile: " 
                                      << tile_msg->fileid << " "
                                      << tile_msg->x << " "
                                      << tile_msg->y << " "
                                      << tile_msg->tilescale
                                      << std::endl;

                          //TileGeneratorThread::request_tile(tile_msg->fileid, tile_msg->pos, tile_msg->tilescale, 
                          //                                  tile_msg->callback);
                        }
                    }
                }
                break;

              default:
                assert(!"Unknown message type");
            }

          delete msg;
        }
      else
        {
          queue.wait();
        }
    }

  return 0;
}

/* EOF */
