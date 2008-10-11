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

#include <iostream>
#include <boost/bind.hpp>
#include <assert.h>
#include "math.hpp"
#include "url.hpp"
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
  DATABASE_THREAD_DONE_MESSAGE,
  DATABASE_REQUEST_FILES_BY_PATTERN_MESSAGE,
  DATABASE_DELETE_FILE_ENTRY_MESSAGE
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
  URL url;
  boost::function<void (FileEntry)> callback;

  FileDatabaseMessage(const URL& url,
                      const boost::function<void (FileEntry)>& callback)
    : DatabaseMessage(DATABASE_FILE_MESSAGE),
      url(url),
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

class FilesByPatternDatabaseMessage : public DatabaseMessage
{
public:
  std::string pattern;
  boost::function<void (FileEntry)> callback;

  FilesByPatternDatabaseMessage(const boost::function<void (FileEntry)>& callback, const std::string& pattern)
    : DatabaseMessage(DATABASE_REQUEST_FILES_BY_PATTERN_MESSAGE),
      pattern(pattern),
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

class DeleteFileEntryDatabaseMessage : public DatabaseMessage
{
public:
  uint32_t fileid;

  DeleteFileEntryDatabaseMessage(uint32_t fileid)
    : DatabaseMessage(DATABASE_DELETE_FILE_ENTRY_MESSAGE),
      fileid(fileid)
  {}
};

DatabaseThread* DatabaseThread::current_ = 0;

DatabaseThread::DatabaseThread(const std::string& filename_)
  : Thread("DatabaseThread"),
    database_filename(filename_),
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
DatabaseThread::request_file(const URL& url, const boost::function<void (FileEntry)>& callback)
{
  queue.push(new FileDatabaseMessage(url, callback));
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
  queue.push(new StoreTileDatabaseMessage(tile));
}

void
DatabaseThread::delete_file_entry(uint32_t fileid)
{
  queue.push(new DeleteFileEntryDatabaseMessage(fileid));
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
                  
                  if (0)
                  std::cout << "Received Tile: "
                            << tile_msg->tile.get_fileid() << " pos: " 
                            << tile_msg->tile.get_pos()    << " scale: " 
                            << tile_msg->tile.get_scale()  << std::endl;

                  for(std::list<TileDatabaseMessage*>::iterator i = tile_queue.begin(); i != tile_queue.end();)
                    {
                      if (tile_msg->tile.get_fileid() == (*i)->file_entry.get_fileid() &&
                          tile_msg->tile.get_scale()  == (*i)->tilescale &&
                          tile_msg->tile.get_pos()    == (*i)->pos)
                        {
                          (*i)->callback(tile_msg->tile);

                          // FIXME: Correct!?
                          delete *i;
                          i = tile_queue.erase(i);
                        }
                      else
                        {
                          ++i;
                        }
                    }
                  
                  // FIXME: Make some better error checking in case of loading failure
                  if (tile_msg->tile.get_software_surface())
                    {
                      // FIXME: Need file_entry object for this:
                      //if (tile_msg->tile.scale == file_entry.thumbnail_scale)
                        //  file_db.store_tile(tile_msg->tile);
                      //else
                        tile_db.store_tile(tile_msg->tile);
                    }
                  else
                    {
                      
                    }
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
                  FileEntry entry = file_db.get_file_entry(file_msg->url);
                  if (!entry)
                    {
                      std::cout << "Error: Couldn't get FileEntry for " << file_msg->url << std::endl;
                    }
                  else
                    {
                      file_msg->callback(entry);
                    }
                }
                break;

              case DATABASE_REQUEST_FILES_BY_PATTERN_MESSAGE:
                {
                  FilesByPatternDatabaseMessage* all_files_msg = static_cast<FilesByPatternDatabaseMessage*>(msg);
                  std::vector<FileEntry> entries;
                  file_db.get_file_entries(entries, all_files_msg->pattern);
                  for(std::vector<FileEntry>::iterator i = entries.begin(); i != entries.end(); ++i)
                    {
                      all_files_msg->callback(*i);
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

              case DATABASE_DELETE_FILE_ENTRY_MESSAGE:
                {
                  DeleteFileEntryDatabaseMessage* delete_msg = static_cast<DeleteFileEntryDatabaseMessage*>(msg);
                  SQLiteStatement(&db)
                    .prepare("BEGIN;")
                    .execute();
                  SQLiteStatement(&db)
                    .prepare("DELETE FROM files WHERE fileid = ?1;")
                    .bind_int(1, delete_msg->fileid)
                    .execute();
                  SQLiteStatement(&db)
                    .prepare("DELETE FROM tiles WHERE fileid = ?1;")
                    .bind_int(1, delete_msg->fileid)
                    .execute();
                  SQLiteStatement(&db)
                    .prepare("END;")
                    .execute();
                }
                break;

              case DATABASE_TILE_MESSAGE:
                {
                  TileDatabaseMessage* tile_msg = static_cast<TileDatabaseMessage*>(msg);

                  if (!tile_msg->job_handle.is_aborted())
                    {
                      TileEntry tile;
                      if (tile_db.get_tile(tile_msg->file_entry.get_fileid(), tile_msg->tilescale, tile_msg->pos, tile))
                        {
                          tile_msg->callback(tile);
                          tile_msg->job_handle.finish();
                        }
                      else
                        {
                          if (0)
                            std::cout << "Error: Couldn't get tile: " 
                                      << tile_msg->file_entry.get_fileid() << " "
                                      << tile_msg->pos.x << " "
                                      << tile_msg->pos.y << " "
                                      << tile_msg->tilescale
                                      << std::endl;
                          
                          tile_queue.push_back(tile_msg);

                          //std::cout << tile_queue.size() << std::endl;
                          msg = 0; // FIXME: HACK so that msg doesn't get deleted, used shared_ptr instead
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

      //std::cout << tile_queue.size() << " vs " << queue.size() << std::endl;

      // Check if job is still valid before starting to generate tiles
      for(std::list<TileDatabaseMessage*>::iterator i = tile_queue.begin(); i != tile_queue.end();)
        {
          if ((*i)->job_handle.is_aborted())
            {
              delete *i;
              i = tile_queue.erase(i);
            }
          else
            {
              ++i;
            }
        }

      if (queue.empty() && // FIXME UGLY: to make load on demand somewhat usable
          TileGeneratorThread::current() && !TileGeneratorThread::current()->is_working())
        {
          if (!tile_queue.empty())
            {
              // FIXME: Either buggy or not syncronized
              TileDatabaseMessage& msg = *tile_queue.back();

              int max_scale = msg.tilescale;

              bool tiles_missing;
              do
                {
                  tiles_missing = false;

                  int width  = Math::ceil_div(msg.file_entry.get_width()  / Math::pow2(max_scale), 256);
                  int height = Math::ceil_div(msg.file_entry.get_height() / Math::pow2(max_scale), 256);

                  for(int y = 0; y < height; ++y)
                    for(int x = 0; x < width; ++x)
                      {
                        if (!tile_db.has_tile(msg.file_entry.get_fileid(), Vector2i(x,y), max_scale+1))
                          {
                            tiles_missing = true;
                            max_scale += 1;
                            goto here;
                          }
                      }

                here:
                  ;
                }
              while(tiles_missing && max_scale < msg.file_entry.get_thumbnail_scale());
              
              TileGeneratorThread::current()->request_tiles(msg.file_entry,
                                                            msg.tilescale,
                                                            max_scale,
                                                            boost::bind(&DatabaseThread::receive_tile, this, _1));
            }
        }
    }

  return 0;
}

/* EOF */
