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

#ifndef HEADER_DATABASE_THREAD_HPP
#define HEADER_DATABASE_THREAD_HPP

#include <boost/function.hpp>
#include <list>
#include <string>
#include "thread_message_queue.hpp"
#include "file_entry.hpp"
#include "tile_entry.hpp"
#include "job_handle.hpp"
#include "thread.hpp"

class URL;
class DatabaseMessage;
class TileDatabaseMessage;

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
  std::list<TileDatabaseMessage*>    tile_queue;

protected: 
  int run();

public:
  DatabaseThread(const std::string&);
  virtual ~DatabaseThread();
  
  void stop();
  
  /* @{ */ // syncronized functions to be used by other threads
  /** Request the tile for file \a tileid */
  JobHandle request_tile(const FileEntry&, int tilescale, const Vector2i& pos, const boost::function<void (TileEntry)>& callback);

  /** Request the FileEntry for \a filename */
  void      request_file(const URL& url, const boost::function<void (FileEntry)>& callback);

  /** Request FileEntrys by glob pattern from the database */
  void      request_files_by_pattern(const boost::function<void (FileEntry)>& callback, const std::string& pattern);

  /** Request all FileEntrys available in the database */
  void      request_all_files(const boost::function<void (FileEntry)>& callback);

  /** Place tile into the database */
  void      receive_tile(const TileEntry& tile);

  /** A Worker Thread reports that it is done and ready to accept new jobs */
  void      receive_job_finished(int threadid);
  /* @} */

private:
  DatabaseThread (const DatabaseThread&);
  DatabaseThread& operator= (const DatabaseThread&);
};

#endif

/* EOF */
