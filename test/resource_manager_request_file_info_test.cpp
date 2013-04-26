#include <thread>

#include "archive/archive_manager.hpp"
#include "database/database.hpp"
#include "galapix/database_thread.hpp"
#include "generator/generator.hpp"
#include "job/job_manager.hpp"
#include "network/download_manager.hpp"
#include "resource/file_info.hpp"
#include "resource/resource_manager.hpp"
#include "resource/resource_locator.hpp"

int main(int argc, char** argv)
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
  for(int i = 1; i < argc; ++i)
  {
    resource_mgr.request_file_info(argv[i], [&count](const Failable<FileInfo>& data){
        try
        {
          FileInfo info = data.get();
          std::cout << "FileInfo retrieved:\n"
                    << "  rowid : ";
          if (info.get_id()) { std::cout << info.get_id().get_id(); } else { std::cout << "<invalid>"; } std::cout << '\n';
          std::cout << "  path  : " << info.get_path() << '\n'
                    << "  mtime : " << info.get_mtime() << '\n'
                    << "  sha1  : " << info.get_sha1() << std::endl;
        }
        catch(const std::exception& err)
        {
          std::cout << "error: " << err.what() << std::endl;
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
