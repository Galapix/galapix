// Galapix - an image viewer for large image collections
// Copyright (C) 2008-2019 Ingo Ruhnke <grumbel@gmail.com>
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "resource/blob_manager.hpp"

#include <fmt/format.h>

#include "archive/archive_manager.hpp"
#include "network/download_manager.hpp"
#include "resource/resource_locator.hpp"

BlobManager::BlobManager(DownloadManager& download_mgr,
                         ArchiveManager&  archive_mgr) :
  m_download_mgr(download_mgr),
  m_archive_mgr(archive_mgr),
  m_pool(4),
  m_mutex(),
  m_requests()
{
}

BlobManager::~BlobManager()
{
  // FIXME: keep track of all async requests and only close down when all of them are done
}

void
BlobManager::request_blob(const ResourceLocator& locator,
                          const std::function<void (Failable<BlobAccessorPtr>)>& callback)
{
  // register_request()

  ResourceURL const& url = locator.get_url();
  if (url.get_scheme() == "file")
  {
    m_pool.schedule
      ([this, locator, url, callback]
       {
         try
         {
           BlobAccessorPtr blob = std::make_shared<BlobAccessor>(url.get_path());
           for(const auto& handler : locator.get_handler())
           {
             assert(blob->has_stdio_name());
             blob = std::make_unique<BlobAccessor>(m_archive_mgr.get_file(blob->get_stdio_name(),
                                                                          handler.get_name(),
                                                                          handler.get_args()));
           }
           callback(blob);
         }
         catch(...)
         {
           callback(Failable<BlobAccessorPtr>(std::current_exception()));
         }
       });
  }
  else if (url.get_scheme() == "http"  ||
           url.get_scheme() == "https" ||
           url.get_scheme() == "ftp")
  {
    m_download_mgr.request_get
      (url.str(),
       [this, callback, locator, url](DownloadResult result)
       {
         if (!result.success())
         {
           callback(Failable<BlobAccessorPtr>::from_exception(std::runtime_error(fmt::format("{}: error: invalid response code: {}",
                                                                                             url.str(), result.get_response_code()))));
         }
         else
         {
           m_pool.schedule
             ([this, callback, locator, result = std::move(result)]() mutable
              {
                try
                {
                  BlobAccessorPtr blob = std::make_shared<BlobAccessor>(result.move_data());
                  for(const auto& handler : locator.get_handler())
                  {
                    assert(blob->has_stdio_name());
                    blob = std::make_shared<BlobAccessor>(m_archive_mgr.get_file(blob->get_stdio_name(),
                                                                                 handler.get_name(), handler.get_args()));
                  }
                  callback(Failable<BlobAccessorPtr>(blob));
                }
                catch(...)
                {
                  callback(Failable<BlobAccessorPtr>(std::current_exception()));
                }
              });
         }
       });
  }
  else
  {
    Failable<BlobAccessorPtr> failable;
    failable.set_exception(std::make_exception_ptr(std::runtime_error(fmt::format("{}: error: unsupported URL scheme: {}",
                                                                                  locator.str(), url.get_scheme()))));
    callback(failable);
  }
}

/* EOF */
