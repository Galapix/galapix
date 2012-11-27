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

#ifndef HEADER_GALAPIX_GALAPIX_GENERATOR_THREAD_HPP
#define HEADER_GALAPIX_GALAPIX_GENERATOR_THREAD_HPP

#include <functional>

struct ResourceInfoResponse
{
  ResourceLocator locator;
  ResourceName    name;
  std::vector<ResourceLocator> children;
};

struct ResourceInfoJob
{
  ResourceLocator locator;
};

class GeneratorThread
{
private:
  ThreadMessageQueue2<std::function<void ()> > m_queue;

public:
  GeneratorThread();

  void request_resource_info(const ResourceLocator& resloc, std::function<void (ResourceInfoResponse)> callback)
  {
    // ask DatabaseThread
    // if ok, return to callback
    // if fail, launch generation
    // when done, store in database and do callback()

    m_queue.push([this]{ process_resource_info_request(resloc, callback); });
    JobHandle handle = m_job_manager.request(ResourceInfoJob());
  }

private:
  GeneratorThread(const GeneratorThread&);
  GeneratorThread& operator=(const GeneratorThread&);
};

#endif

/* EOF */
