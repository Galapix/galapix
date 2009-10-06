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

//#include <boost/function.hpp>
//#include <list>
//#include <string>
#include "job/thread.hpp"
#include "job/thread_message_queue.hpp"
//#include "file_entry.hpp"
#include "database/tile_entry.hpp"
#include "job/job_handle.hpp"

class URL;
class DatabaseMessage;
class TileDatabaseMessage;
class FileEntry;

class DatabaseThread : public Thread
{
private:
  static DatabaseThread* current_;
public:
  static DatabaseThread* current() { return current_; }
  
private:
  std::string database_filename;
  bool abort_instantly;
  bool quit;
  
  ThreadMessageQueue<DatabaseMessage*> queue;

  struct TileRequest
  {
    JobHandle job_handle;
    int scale;
    Vector2i pos;
    boost::function<void (TileEntry)> callback;
    
    TileRequest(const JobHandle& job_handle_,
                int scale_, const Vector2i& pos_,
                const boost::function<void (TileEntry)>& callback_)
      : job_handle(job_handle_),
        scale(scale_), pos(pos_),
        callback(callback_)
    {}
  };

  struct TileRequestGroup 
  {
    TileRequestGroup(int64_t fileid_, int min_scale_, int max_scale_)
      : fileid(fileid_),
        min_scale(min_scale_),
        max_scale(max_scale_)
    {}
    
    int64_t fileid; 
    int min_scale;
    int max_scale;
    std::vector<TileRequest> requests;
  };
  
  typedef std::list<TileRequestGroup> TileRequestGroups;
  TileRequestGroups tile_request_groups;

protected: 
  void run();

public:
  DatabaseThread(const std::string&);
  virtual ~DatabaseThread();

  void stop_thread();

  void process_tile(const TileEntry& tile_entry);
  void generate_tile(const JobHandle& job_handle,
                     const FileEntry&, int tilescale, const Vector2i& pos, 
                     const boost::function<void (TileEntry)>& callback);
  
  /* @{ */ // syncronized functions to be used by other threads
  /** Request the tile for file \a tileid */
  JobHandle request_tile(const FileEntry&, int tilescale, const Vector2i& pos, const boost::function<void (TileEntry)>& callback);

  /** Request the FileEntry for \a filename */
  JobHandle request_file(const URL& url, const boost::function<void (const FileEntry&)>& callback);

  /** Request FileEntrys by glob pattern from the database */
  void      request_files_by_pattern(const boost::function<void (FileEntry)>& callback, const std::string& pattern);

  /** Request all FileEntrys available in the database */
  void      request_all_files(const boost::function<void (FileEntry)>& callback);

  /** Place tile into the database */
  void      receive_tile(const TileEntry& tile);
  void      receive_tiles(const std::vector<TileEntry>& tiles);

  /** Delete the given FileEntry along with all TileEntry refering to it */
  void      delete_file_entry(int64_t fileid);
  /* @} */

private:
  DatabaseThread (const DatabaseThread&);
  DatabaseThread& operator= (const DatabaseThread&);
};

#endif

/* EOF */
