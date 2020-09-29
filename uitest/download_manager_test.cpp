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

#include <logmich/log.hpp>
#include <uitest/uitest.hpp>
#include <strut/tokenize.hpp>
#include <strut/from_string.hpp>

#include "network/download_manager.hpp"
#include "network/download_progress.hpp"
#include "network/download_result.hpp"

namespace {

void run_repl()
{
  DownloadManager downloader;

  std::cout << "Commands: get, post, cancel, cancel_all" << std::endl;

  std::string line;
  while(std::getline(std::cin, line))
  {
    std::vector<std::string> args = strut::tokenize(line, ' ');

    if (!args.empty())
    {
      if (args[0] == "get")
      {
        if (args.size() != 2)
        {
          std::cout << "'get' requires two arguments" << std::endl;
        }
        else
        {
          auto handle =
            downloader.request_get(args[1],
                                   [=](const DownloadResult& result) {

                                     std::cout << "results for " << args[1] << std::endl;
                                     std::cout << "  response code     : " << result.get_response_code() << std::endl;
                                     std::cout << "  modification time : " << result.get_mtime() << std::endl;

                                     if (result.success())
                                     {
                                       std::cout.write(reinterpret_cast<const char*>(result.get_data().data()),
                                                       static_cast<std::streamsize>(result.get_data().size()));
                                     }
                                     else
                                     {
                                       std::cout << "failure: " << result.get_response_code() << std::endl;
                                     }
                                   },
                                   [=](DownloadProgress const& progress) -> bool {
                                     std::cout << args[1] << ": " << progress.dlnow/1000 << " / " << progress.dltotal/1000 << std::endl;
                                     return false;
                                   });
          std::cout << "launched " << handle << std::endl;
        }
      }
      else if (args[0] == "post")
      {
        if (args.size() != 3)
        {
          std::cout << "'post' requires three arguments" << std::endl;
        }
        else
        {
          auto handle =
            downloader.request_post(args[1],
                                    args[2],
                                    [=](const DownloadResult& result) {
                                      std::cout << "got " << result.get_response_code() << " for " << " " << args[1] << std::endl;
                                      if (result.success())
                                      {
                                        std::cout.write(reinterpret_cast<const char*>(result.get_data().data()),
                                                        static_cast<std::streamsize>(result.get_data().size()));
                                      }
                                      else
                                      {
                                        std::cout << "failure: " << result.get_response_code() << std::endl;
                                      }
                                    },
                                    [=](DownloadProgress progress) -> bool {
                                      std::cout << args[1] << ": " << progress.dlnow/1000 << " / " << progress.dltotal/1000 << std::endl;
                                      return false;
                                    });
          std::cout << "launched " << handle << std::endl;
        }
      }
      else if (args[0] == "cancel_all")
      {
        downloader.cancel_all_transfers();
      }
      else if (args[0] == "cancel")
      {
        if (args.size() != 2)
        {
          std::cout << "'cancel' requires one arguments" << std::endl;
        }
        else
        {
          DownloadManager::TransferHandle handle;
          if (strut::from_string(args[1], handle))
          {
            downloader.cancel_transfer(handle);
          }
          else
          {
            std::cout << "not a valid TransferHandle: " << args[1] << std::endl;
          }
        }
      }
      else
      {
        std::cout << "unknown command: " << args[0] << std::endl;
      }
    }
  }

}

} // namespace

UITEST(DownloadManager, test, "",
       "Run interactive download shell")
{
  logmich::set_log_level(logmich::kDebug);
  run_repl(); // NOLINT
}

/* EOF */
