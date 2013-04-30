#include <thread>

#include "archive/archive_manager.hpp"
#include "network/download_manager.hpp"
#include "resource/blob_manager.hpp"
#include "resource/resource_locator.hpp"

int main(int argc, char** argv)
{
  DownloadManager download_mgr;
  ArchiveManager  archive_mgr;

  BlobManager blob_mgr(download_mgr, archive_mgr);
  
  int count = 0;
  for(int i = 1; i < argc; ++i)
  {
    ResourceLocator locator = ResourceLocator::from_string(argv[i]);
    std::cout << "requesting " << locator.str() << std::endl;
    blob_mgr.request_blob
      (locator, 
       [&count](const Failable<BlobPtr>& data)
       {
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

  return 0;
}

/* EOF */
