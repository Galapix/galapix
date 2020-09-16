#include <thread>
#include <uitest/uitest.hpp>

#include "archive/archive_manager.hpp"
#include "network/download_manager.hpp"
#include "resource/blob_manager.hpp"
#include "resource/resource_locator.hpp"

UITEST(BlobManager, request_blob, "BLOB...",
       "Request a blob")
{
  DownloadManager download_mgr;
  ArchiveManager  archive_mgr;

  BlobManager blob_mgr(download_mgr, archive_mgr);

  std::atomic_int count = 0;
  for(const auto& arg : rest)
  {
    ResourceLocator locator = ResourceLocator::from_string(arg);
    std::cout << "requesting " << locator.str() << std::endl;
    blob_mgr.request_blob
      (locator,
       [&count](const Failable<BlobAccessorPtr>& data)
       {
         try
         {
           std::cout << "data retrieved: " << data.get()->get_blob().size() << std::endl;
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
}

/* EOF */
