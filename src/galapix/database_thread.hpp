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
#include <boost/optional.hpp>

#include "database/entries/tile_entry.hpp"
#include "database/entries/resource_entry.hpp"
#include "galapix/tile.hpp"
#include "job/job_handle.hpp"
#include "job/job_manager.hpp"
#include "job/thread.hpp"
#include "job/thread_message_queue2.hpp"
#include "util/failable.hpp"

class BlobInfo;
class Database;
class DatabaseMessage;
class FileInfo;
class ImageInfo;
class OldFileEntry;
class ResourceInfo;
class ResourceLocator;
class TileDatabaseMessage;
class TileGenerationJob;
class URL;
class URLInfo;

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

  ThreadMessageQueue2<std::function<void()>> m_request_queue;
  ThreadMessageQueue2<std::function<void()>> m_receive_queue;
  std::list<std::shared_ptr<TileGenerationJob> > m_tile_generation_jobs;

protected:
  void run();

public:
  DatabaseThread(Database& database,
                 JobManager& tile_job_manager);
  virtual ~DatabaseThread();

  void stop_thread();
  void abort_thread();

  void request_resource_metadata();

  void request_image_info(const ResourceInfo& resource, const std::function<void (const boost::optional<ImageInfo>&)>& callback);
  void store_image_info(const ImageInfo& image_info,
                       const std::function<void (const Failable<ImageInfo>&)>& callback);

  void request_resource_info(const ResourceLocator& locator, const BlobInfo& blob,
                             const std::function<void (const boost::optional<ResourceInfo>&)>& callback);
  void store_resource_info(const ResourceInfo& resource_info,
                           const std::function<void (const Failable<ResourceInfo>&)>& callback);

  void request_file_info(const std::string& path, const std::function<void (const boost::optional<FileInfo>&)>& callback);
  void store_file_info(const FileInfo& file_info,
                       const std::function<void (const Failable<FileInfo>&)>& callback);

  void request_url_info(const std::string& url, const std::function<void (const boost::optional<URLInfo>&)>& callback);
  void store_url_info(const URLInfo& url_info,
                      const std::function<void (const Failable<URLInfo>&)>& callback);

  /* @{ */ // syncronized functions to be used by other threads
  /**
   *  Request the tile from the database, if not in the database the
   *  tile will be generated from the source image
   */
  JobHandle request_tile(const OldFileEntry&, int tilescale, const Vector2i& pos,
                         const std::function<void (Tile)>& callback);

  JobHandle request_tiles(const OldFileEntry&, int min_scale, int max_scale,
                          const std::function<void (Tile)>& callback);

  void      request_job_removal(std::shared_ptr<Job> job, bool);

  /** Request the OldFileEntry for \a filename */
  JobHandle request_file(const URL& url,
                         const std::function<void (OldFileEntry)>& file_callback);

  /** Request OldFileEntrys by glob pattern from the database */
  void      request_files_by_pattern(const std::function<void (OldFileEntry)>& callback, const std::string& pattern);

  /** Request all OldFileEntrys available in the database */
  void      request_all_files(const std::function<void (OldFileEntry)>& callback);

  void      store_file_entry(const JobHandle& job_handle,
                             const URL& url, int size, int mtime, OldFileEntry::Handler handler,
                             const std::function<void (OldFileEntry)>& callback);

  /** Delete the given OldFileEntry along with all TileEntry refering to it */
  void      delete_file_entry(const RowId& fileid);
  /* @} */

  void request_resource_entry(const RowId& blob_id,
                              const std::function<void (const boost::optional<ResourceEntry>&)>& callback);

private:
  void remove_job(std::shared_ptr<Job> job);

  /** Generates the requested tile from its original image */
  void generate_tiles(const JobHandle& job_handle, const OldFileEntry&,
                      int min_scale, int max_scale,
                      const std::function<void (Tile)>& callback);

  /** Generates the requested tile from its original image */
  void generate_tile(const JobHandle& job_handle,
                     const OldFileEntry&, int tilescale, const Vector2i& pos,
                     const std::function<void (Tile)>& callback);

  void generate_file_entry(const JobHandle& job_handle, const URL& url,
                           const std::function<void (OldFileEntry)>& file_callback);

  /** Place tile into the database */
  void receive_tile(const RowId& fileid, const Tile& tile);
  void receive_file(const OldFileEntry& file_entry);
  void receive_tiles(const std::vector<TileEntry>& tiles);

  void process_queue(ThreadMessageQueue2<std::function<void()>>& queue);

private:
  DatabaseThread (const DatabaseThread&);
  DatabaseThread& operator= (const DatabaseThread&);
};

#endif

/* EOF */
