/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmx.de>
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
#include "network/download_result.hpp"
#include <logmich/log.hpp>

UITEST(DownloadManager, post, "URL...",
       "Post to an URL")
{
  logmich::set_log_level(logmich::kDebug);
  DownloadManager downloader;

  for(const auto& arg : args)
  {
    std::cout << "sending request: " << arg << std::endl;
    downloader.request_post(arg,
                            "Hello World",
                            [=](const DownloadResult& result) {
                              std::cout << "got " << result.get_response_code() << " for " << " " << arg << std::endl;
                              if (result.success())
                              {
                                std::cout.write(reinterpret_cast<const char*>(result.get_blob()->get_data()),
                                                result.get_blob()->size());
                              }
                              else
                              {
                                std::cout << "failure: " << result.get_response_code() << std::endl;
                              }
                            },
                            [=](double dltotal, double dlnow, double ultotal, double ulnow) -> bool {
                              std::cout << arg << ": " << dlnow/1000 << " / " << dltotal/1000 << std::endl;
                              return false;
                            });
  }
}

/* EOF */
