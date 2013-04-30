/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmx.de>
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

#include "archive/archive_manager.hpp"
#include "database/database.hpp"
#include "galapix/database_thread.hpp"
#include "generator/generator.hpp"
#include "job/job_manager.hpp"
#include "network/download_manager.hpp"
#include "resource/blob_manager.hpp"
#include "resource/file_info.hpp"
#include "resource/resource_locator.hpp"
#include "resource/resource_manager.hpp"

int main(int argc, char** argv)
{
  g_logger.set_log_level(Logger::kDebug);

  Database db("/tmp/resource_manager_test");
  JobManager job_manager(4);

  job_manager.start_thread();

  DatabaseThread database(db, job_manager);
  DownloadManager download_mgr;
  ArchiveManager archive_mgr;
  BlobManager blob_mgr(download_mgr, archive_mgr);
  Generator generator(blob_mgr);

  database.start_thread();

  ResourceManager resource_mgr(database, generator, download_mgr, archive_mgr);
  
  int count = 0;
  for(int i = 1; i < argc; ++i)
  {
    ResourceLocator locator = ResourceLocator::from_string(argv[i]);

    resource_mgr.request_resource_info
      (locator, [&count](const Failable<ResourceInfo>& data)
       {
         try
         {
           ResourceInfo info = data.get();
           std::cout << "ResourceInfo retrieved:\n"
                     << "  rowid : ";
           if (info.get_id()) { std::cout << info.get_id().get_id(); } else { std::cout << "<invalid>"; } std::cout << '\n';
           std::cout << "  status  : " << to_string(info.get_status()) << '\n'
                     << "  blob    : " << info.get_blob().get_sha1().str() << " " << info.get_blob().get_size() << '\n'
                     << "  handler : " << info.get_handler().str() << '\n';
         }
         catch(const std::exception& err)
         {
           std::cout << "Error: " << err.what() << std::endl;
         }
         count -= 1;
       });
    count += 1;
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

  return 0;
}

/* EOF */
