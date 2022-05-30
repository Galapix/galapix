/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmail.com>
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

#include <thread>
#include <uitest/uitest.hpp>

#include <arxp/archive_manager.hpp>

#include "database/database.hpp"
#include "generator/generator.hpp"
#include "job/job_manager.hpp"
#include "network/download_manager.hpp"
#include "resource/blob_manager.hpp"
#include "resource/file_info.hpp"
#include "resource/resource_locator.hpp"
#include "resource/resource_manager.hpp"
#include "server/database_thread.hpp"

UITEST(ResourceManager, request_url_info, "URL...")
{
  logmich::set_log_level(logmich::LogLevel::DEBUG);

  Database db = Database::create("/tmp/resource_manager_test");
  JobManager job_manager(4);

  job_manager.start_thread();

  DatabaseThread database(db, job_manager);
  DownloadManager download_mgr;
  arxp::ArchiveManager archive_mgr;
  BlobManager blob_mgr(download_mgr, archive_mgr);
  Generator generator(blob_mgr, archive_mgr);

  database.start_thread();

  ResourceManager resource_mgr(database, generator, download_mgr, archive_mgr);

  std::atomic_int count = 0;
  //for(const auto& arg : rest)
  {
#if 0
    std::string url = arg;

    resource_mgr.request_url_info
      (url,
       [&count, url](const Failable<URLInfo>& data)
       {
        try
        {
          const URLInfo& info = data.get();
          std::cout << "URL    : " << url << std::endl;
          std::cout << "SHA1   : " << info.get_blob_info().get_sha1().str() << std::endl;
          std::cout << "size   : " << info.get_blob_info().get_size() << std::endl;
          std::cout << "mtime  : " << info.get_mtime() << std::endl;
          std::cout << "content_type : " << info.get_content_type() << std::endl;
        }
        catch(const std::exception& err)
        {
          std::cout << "error: " << err.what() << std::endl;
        }
        count -= 1;
      });
    count += 1;
#endif
  }

  log_debug("going into loop");
  while(count > 0)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  log_debug("going out of loop");

  database.abort_thread();
  database.join_thread();

  job_manager.stop_thread();
  job_manager.join_thread();
}

/* EOF */
