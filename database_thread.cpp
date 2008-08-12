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
#include "sqlite.h"
#include "file_database.hpp"
#include "tile_database.hpp"
#include "database_thread.hpp"

enum DatabaseMessageType 
{
  DATABASE_FILE_MESSAGE,
  DATABASE_TILE_MESSAGE,
};

class DatabaseMessage
{
public:
  DatabaseMessageType type;

  DatabaseMessage(DatabaseMessageType type)
    : type(type)
  {}

  virtual ~DatabaseMessage()
  {}
};

class TileDatabaseMessage : public DatabaseMessage
{
public:
  int fileid;
  int tilescale;
  int x;
  int y;
  boost::function<void (Tile)> callback;

  TileDatabaseMessage(int fileid, int tilescale, int x, int y,
                      boost::function<void (Tile)> callback)
    : DatabaseMessage(DATABASE_TILE_MESSAGE),
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
                      boost::function<void (FileEntry)> callback)
    : DatabaseMessage(DATABASE_FILE_MESSAGE),
      filename(filename),
      callback(callback)
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

void
DatabaseThread::request_tile(int fileid, int tilescale, int x, int y, boost::function<void (Tile)> callback)
{
  queue.push(new TileDatabaseMessage(fileid, tilescale, x, y, callback));
}

void
DatabaseThread::request_file(const std::string& filename, boost::function<void (FileEntry)> callback)
{
  queue.push(new FileDatabaseMessage(filename, callback));
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

  while(!quit)
    {
      //std::cout << "DatabaseThread: looping" << std::endl;
      
      // do things
      while(!queue.empty() && !quit)
        {
          DatabaseMessage* msg = queue.front();
          queue.pop();

          switch(msg->type)
            {
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

              case DATABASE_TILE_MESSAGE:
                {
                   TileDatabaseMessage* tile_msg = static_cast<TileDatabaseMessage*>(msg);
                   Tile tile;
                   if (tile_db.get_tile(tile_msg->fileid, tile_msg->tilescale, tile_msg->x, tile_msg->y, tile))
                     {
                       tile_msg->callback(tile);
                     }
                   else
                     {
                       std::cout << "Error: Couldn't get tile: " 
                                 << tile_msg->fileid << " "
                                 << tile_msg->x << " "
                                 << tile_msg->y << " "
                                 << tile_msg->tilescale
                                 << std::endl;
                     }
                }
                break;

              default:
                assert(!"Unknown message type");
            }

          delete msg;
        }
      
      queue.wait();
    }

  return 0;
}

/* EOF */
