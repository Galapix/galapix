// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef HEADER_GALAPIX_SERVER_DATABASE_THREAD_HPP
#define HEADER_GALAPIX_SERVER_DATABASE_THREAD_HPP

#include <list>
#include <optional>

#include "database/entries/tile_entry.hpp"
#include "database/entries/resource_entry.hpp"
#include "galapix/tile.hpp"
#include "job/job_handle.hpp"
#include "job/job_manager.hpp"
#include "job/thread.hpp"
#include "job/thread_message_queue2.hpp"
#include "util/failable.hpp"

namespace galapix {

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

public:
  DatabaseThread(Database& database,
                 JobManager& tile_job_manager);
  ~DatabaseThread() override;

  void stop_thread() override;
  void abort_thread();

  void request_resource_metadata();

  void request_image_info(ResourceInfo const& resource, const std::function<void (std::optional<ImageInfo> const&)>& callback);
  void store_image_info(ImageInfo const& image_info,
                       const std::function<void (Failable<ImageInfo> const&)>& callback);

  void request_resource_info(ResourceLocator const& locator, BlobInfo const& blob,
                             const std::function<void (std::optional<ResourceInfo> const&)>& callback);
  void store_resource_info(ResourceInfo const& resource_info,
                           const std::function<void (Failable<ResourceInfo> const&)>& callback);

  void request_file_info(std::string const& path, const std::function<void (std::optional<FileInfo> const&)>& callback);
  void store_file_info(FileInfo const& file_info,
                       const std::function<void (Failable<FileInfo> const&)>& callback);

  void request_url_info(std::string const& url, const std::function<void (std::optional<URLInfo> const&)>& callback);
  void store_url_info(URLInfo const& url_info,
                      const std::function<void (Failable<URLInfo> const&)>& callback);

  /* @{ */ // syncronized functions to be used by other threads
  /**
   *  Request the tile from the database, if not in the database the
   *  tile will be generated from the source image
   */
  JobHandle request_tile(OldFileEntry const& file_entry, int tilescale, Vector2i const& pos,
                         const std::function<void (Tile)>& callback);

  JobHandle request_tiles(OldFileEntry const& file_entry, int min_scale, int max_scale,
                          const std::function<void (Tile)>& callback);

  void request_job_removal(std::shared_ptr<Job> const& job, bool unused);

  /** Request the OldFileEntry for \a filename */
  JobHandle request_file(URL const& url,
                         const std::function<void (OldFileEntry)>& file_callback);

  /** Request OldFileEntrys by glob pattern from the database */
  void request_files_by_pattern(const std::function<void (OldFileEntry)>& callback, std::string const& pattern);

  /** Request all OldFileEntrys available in the database */
  void request_all_files(const std::function<void (OldFileEntry)>& callback);

  void store_file_entry(JobHandle const& job_handle,
                        URL const& url, int size, int mtime, OldFileEntry::Handler handler,
                        const std::function<void (OldFileEntry)>& callback);

  /** Delete the given OldFileEntry along with all TileEntry refering to it */
  void      delete_file_entry(RowId const& fileid);
  /* @} */

  void request_resource_entry(RowId const& blob_id,
                              const std::function<void (std::optional<ResourceEntry> const&)>& callback);

private:
  void remove_job(std::shared_ptr<Job> const& job);

  /** Generates the requested tile from its original image */
  void generate_tiles(JobHandle const& job_handle, OldFileEntry const& file_entry,
                      int min_scale, int max_scale,
                      const std::function<void (Tile)>& callback);

  /** Generates the requested tile from its original image */
  void generate_tile(JobHandle const& job_handle,
                     OldFileEntry const& file_entry, int tilescale, Vector2i const& pos,
                     const std::function<void (Tile)>& callback);

  void generate_file_entry(JobHandle const& job_handle, URL const& url,
                           const std::function<void (OldFileEntry)>& file_callback);

  /** Place tile into the database */
  void receive_tile(RowId const& fileid, Tile const& tile);
  void receive_file(OldFileEntry const& file_entry);
  void receive_tiles(std::vector<TileEntry> const& tiles);

  void process_queue(ThreadMessageQueue2<std::function<void()>>& queue) const;

protected:
  void run() override;

private:
  Database& m_database;

  JobManager& m_tile_job_manager;

  bool m_quit;
  bool m_abort;

  ThreadMessageQueue2<std::function<void()>> m_request_queue;
  ThreadMessageQueue2<std::function<void()>> m_receive_queue;
  std::list<std::shared_ptr<TileGenerationJob> > m_tile_generation_jobs;

private:
  DatabaseThread (DatabaseThread const&);
  DatabaseThread& operator= (DatabaseThread const&);
};

} // namespace galapix

#endif

/* EOF */
