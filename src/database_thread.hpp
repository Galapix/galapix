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

#ifndef HEADER_DATABASE_THREAD_HPP
#define HEADER_DATABASE_THREAD_HPP

#include <boost/function.hpp>
#include <string>
#include "thread_message_queue.hpp"
#include "file_entry.hpp"
#include "tile_entry.hpp"
#include "job_handle.hpp"
#include "thread.hpp"

class DatabaseMessage;

/** */
class DatabaseThread : public Thread
{
private:
  static DatabaseThread* current_;
public:
  static DatabaseThread* current() { return current_; }
  
private:
  std::string database_filename;
  bool quit;
  
  ThreadMessageQueue<DatabaseMessage*> queue;

protected: 
  int run();

public:
  DatabaseThread(const std::string&);
  virtual ~DatabaseThread();
  
  void stop();
  
  JobHandle request_tile(int fileid, int tilescale, int x, int y, const boost::function<void (TileEntry)>& callback);
  void request_file(const std::string& filename, const boost::function<void (FileEntry)>& callback);
  void request_all_files(const boost::function<void (FileEntry)>& callback);

  void store_tile(const TileEntry& tile);

private:
  DatabaseThread (const DatabaseThread&);
  DatabaseThread& operator= (const DatabaseThread&);
};

#endif

/* EOF */
