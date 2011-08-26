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

#include <list>

#include "database/tile_entry.hpp"
#include "galapix/tile.hpp"
#include "job/job_handle.hpp"
#include "job/job_manager.hpp"
#include "job/thread.hpp"
#include "job/thread_message_queue2.hpp"

class URL;
class Database;
class DatabaseMessage;
class TileDatabaseMessage;
class TileGenerationJob;
class FileEntry;

class DatabaseThread : public Thread
{
private:
  static DatabaseThread* current_;
public:
  static DatabaseThread* current() { return current_; }

private:
  
private:
  Database& m_database;

  JobManager& m_tile_job_manager;

  bool m_quit;
  bool m_abort;
  
  ThreadMessageQueue2<DatabaseMessage*> m_request_queue;
  ThreadMessageQueue2<DatabaseMessage*> m_receive_queue;
  std::list<std::shared_ptr<TileGenerationJob> > m_tile_generation_jobs;

protected: 
  void run();

public:
  DatabaseThread(Database& database,
                 JobManager& tile_job_manager);
  virtual ~DatabaseThread();

  void stop_thread();
  void abort_thread();

  /** Generates the requested tile from its original image */
  void generate_tiles(const JobHandle& job_handle, const FileEntry&,
                      int min_scale, int max_scale,
                      const std::function<void (Tile)>& callback);

  /** Generates the requested tile from its original image */
  void generate_tile(const JobHandle& job_handle,
                     const FileEntry&, int tilescale, const Vector2i& pos, 
                     const std::function<void (Tile)>& callback);

  void generate_file_entry(const JobHandle& job_handle, const URL& url,
                           const std::function<void (FileEntry)>& file_callback,
                           const std::function<void (FileEntry, Tile)>& tile_callback);
  void remove_job(std::shared_ptr<Job> job);

  /* @{ */ // syncronized functions to be used by other threads
  /**
   *  Request the tile from the database, if not in the database the
   *  tile will be generated from the source image
   */
  JobHandle request_tile(const FileEntry&, int tilescale, const Vector2i& pos, 
                         const std::function<void (Tile)>& callback);

  JobHandle request_tiles(const FileEntry&, int min_scale, int max_scale, 
                          const std::function<void (Tile)>& callback);

  void      request_job_removal(std::shared_ptr<Job> job, bool);

  /** Request the FileEntry for \a filename */
  JobHandle request_file(const URL& url, 
                         const std::function<void (FileEntry)>& file_callback,
                         const std::function<void (FileEntry, Tile)>& tile_callback = std::function<void (FileEntry, Tile)>());

  /** Request FileEntrys by glob pattern from the database */
  void      request_files_by_pattern(const std::function<void (FileEntry)>& callback, const std::string& pattern);

  /** Request all FileEntrys available in the database */
  void      request_all_files(const std::function<void (FileEntry)>& callback);

  void      store_file_entry(const JobHandle& job_handle, 
                             const URL& url, const Size& size, int format,
                             const std::function<void (FileEntry)>& callback);

  /** Place tile into the database */
  void      receive_tile(const FileEntry& file_entry, const Tile& tile);
  void      receive_file(const FileEntry& file_entry);
  void      receive_tiles(const std::vector<TileEntry>& tiles);

  /** Delete the given FileEntry along with all TileEntry refering to it */
  void      delete_file_entry(const FileId& fileid);
  /* @} */

private:
  void process_queue(ThreadMessageQueue2<DatabaseMessage*>& queue);

private:
  DatabaseThread (const DatabaseThread&);
  DatabaseThread& operator= (const DatabaseThread&);
};

#endif

/* EOF */
