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

#ifndef HEADER_GALAPIX__HOME_INGO_PROJECTS_GALAPIX_TRUNK_TEST_RESOURCE_MANAGER_TEST_HELPER_HPP
#define HEADER_GALAPIX__HOME_INGO_PROJECTS_GALAPIX_TRUNK_TEST_RESOURCE_MANAGER_TEST_HELPER_HPP

class ResourceManagerTestHelper
{
private:
public:
  ResourceManagerTestHelper()
  {
    g_logger.set_log_level(Logger::kDebug);

    Database db("/tmp/resource_manager_test");
    JobManager job_manager(4);

    job_manager.start_thread();

    DatabaseThread database(db, job_manager);
    Generator generator;
    DownloadManager download_mgr;
    ArchiveManager archive_mgr;

    database.start_thread();

    ResourceManager resource_mgr(database, generator, download_mgr, archive_mgr);

    int count = 0;
    for(const auto& arg : args)
    {
      ResourceLocator locator = ResourceLocator::from_string(arg);

      resource_mgr.request_sha1
        (locator,
         [&count, locator](const Failable<SHA1>& data)
         {
           try
           {
             std::cout << "locator : " << locator.str() << std::endl;
             std::cout << "SHA1    : " << data.get().str() << std::endl;
           }
           catch(const std::exception& err)
           {
             std::cout << "error: " << err.what() << std::endl;
           }
           count -= 1;
         });
      count += 1;
    }

    ~ResourceManagerTestHelper()
    {
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

  private:
    ResourceManagerTestHelper(const ResourceManagerTestHelper&);
    ResourceManagerTestHelper& operator=(const ResourceManagerTestHelper&);
  };

#endif

/* EOF */
