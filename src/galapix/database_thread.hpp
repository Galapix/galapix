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

#ifndef HEADER_GALAPIX_GALAPIX_DATABASE_THREAD_HPP
#define HEADER_GALAPIX_GALAPIX_DATABASE_THREAD_HPP

#include "job/thread.hpp"
#include "job/thread_message_queue.hpp"
#include "database/tile_entry.hpp"
#include "job/job_handle.hpp"
#include "job/job_manager.hpp"

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
  JobManager& m_tile_job_manager;
  JobManager& m_file_entry_job_manager;

  std::string database_filename;
  bool m_quit;
  bool m_abort;
  
  ThreadMessageQueue<DatabaseMessage*> m_queue;

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
    TileRequestGroup(const FileEntry& file_entry_, int min_scale_, int max_scale_)
      : file_entry(file_entry_),
        min_scale(min_scale_),
        max_scale(max_scale_),
        requests()
    {}
    
    FileEntry file_entry; 
    int min_scale;
    int max_scale;
    std::vector<TileRequest> requests;
  };
  
  typedef std::list<TileRequestGroup> TileRequestGroups;
  TileRequestGroups tile_request_groups;

protected: 
  void run();

public:
  DatabaseThread(const std::string& db_filename,
                 JobManager& tile_job_manager,
                 JobManager& file_entry_job_manager);
  virtual ~DatabaseThread();

  void stop_thread();
  void abort_thread();

  void process_tile(const TileEntry& tile_entry);
  void generate_tile(const JobHandle& job_handle,
                     const FileEntry&, int tilescale, const Vector2i& pos, 
                     const boost::function<void (TileEntry)>& callback);

  void generate_file_entry(const JobHandle& job_handle, const URL& url,
                           const boost::function<void (FileEntry)>& callback);
  
  /* @{ */ // syncronized functions to be used by other threads
  /** Request the tile for file \a tileid */
  JobHandle request_tile(const FileEntry&, int tilescale, const Vector2i& pos, const boost::function<void (TileEntry)>& callback);

  /** Request the FileEntry for \a filename */
  JobHandle request_file(const URL& url, const boost::function<void (const FileEntry&)>& callback);

  /** Request FileEntrys by glob pattern from the database */
  void      request_files_by_pattern(const boost::function<void (FileEntry)>& callback, const std::string& pattern);

  /** Request all FileEntrys available in the database */
  void      request_all_files(const boost::function<void (FileEntry)>& callback);

  void      store_file_entry(const URL& url, const Size& size, 
                             const boost::function<void (FileEntry)>& callback);

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
