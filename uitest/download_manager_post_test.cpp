/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmail.com>
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

#include <iostream>
#include <thread>
#include <uitest/uitest.hpp>

#include "network/download_manager.hpp"
#include "network/download_progress.hpp"
#include "network/download_result.hpp"
#include <logmich/log.hpp>

using namespace galapix;

UITEST(DownloadManager, post, "URL...",
       "Post to an URL")
{
  logmich::set_log_level(logmich::LogLevel::DEBUG);
  DownloadManager downloader;

  for(const auto& arg : rest)
  {
    std::cout << "sending request: " << arg << std::endl;
    downloader.request_post(arg,
                            "Hello World",
                            [=](const DownloadResult& result) {
                              std::cout << "got " << result.response_code << " for " << " " << arg << std::endl;
                              if (result.success())
                              {
                                std::cout.write(reinterpret_cast<const char*>(result.data.data()),
                                                static_cast<std::streamsize>(result.data.size()));
                              }
                              else
                              {
                                std::cout << "failure: " << result.response_code << std::endl;
                              }
                            },
                            [=](DownloadProgress const& progress) -> bool {
                              std::cout << arg << ": " << progress.dlnow/1000 << " / " << progress.dltotal/1000 << std::endl;
                              return false;
                            });
  }
}

/* EOF */
