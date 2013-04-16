#include <thread>

#include "archive/archive_manager.hpp"
#include "database/database.hpp"
#include "galapix/database_thread.hpp"
#include "generator/generator.hpp"
#include "job/job_manager.hpp"
#include "network/download_manager.hpp"
#include "resource/resource_manager.hpp"
#include "resource/resource_locator.hpp"

int main(int argc, char** argv)
{
  Database db("/tmp/test.sqlite3");
  JobManager job_manager(4);

  job_manager.start_thread();

  DatabaseThread database(db, job_manager);
  Generator generator(job_manager);
  DownloadManager download_mgr;
  ArchiveManager archive_mgr;

  ResourceManager resource_mgr(database, generator, download_mgr, archive_mgr);
  
  int count = 0;
  for(int i = 1; i < argc; ++i)
  {
    ResourceLocator locator = ResourceLocator::from_string(argv[i]);
    std::cout << "requesting " << locator.str() << std::endl;
    resource_mgr.request_blob(locator, [&count](const Failable<BlobPtr>& data){
        try
        {
          std::cout << "data retrieved: " << data.get()->size() << std::endl;
        }
        catch(const std::exception& err)
        {
          std::cout << "error: " << err.what() << std::endl;
        }
        count -= 1;
      });
    count += 1;
  }

  while(count > 0) 
  { 
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  job_manager.stop_thread();
  job_manager.join_thread();

  return 0;
}

/* EOF */
