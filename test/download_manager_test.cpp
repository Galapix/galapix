#include <gtest/gtest.h>

#include "network/download_manager.hpp"

TEST(DownloadManagerTest, request_get) {
  bool success = false;
  {
    DownloadManager downloader;
    downloader.request_get("https://raw.githubusercontent.com/Galapix/galapix/master/test/jpeg_size_test.cpp",
                           [&success](const DownloadResult& result){
                             if (result.success())
                             {
                               result.get_data();
                               success = true;
                             }
                           });
  }
  ASSERT_TRUE(success);
}

/* EOF */
