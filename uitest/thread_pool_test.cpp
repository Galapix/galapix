/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2013 Ingo Ruhnke <grumbel@gmx.de>
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
#include <uitest/uitest.hpp>

#include "util/thread_pool.hpp"

UITEST(ThreadPool, test, "")
{
  {
    std::cout << "TEST: ThreadPool::~ThreadPool()" << std::endl;
    ThreadPool pool(4);
    std::mutex mutex;

    for(int i = 0; i < 30; ++i)
    {
      pool.schedule([i, &mutex]{
          std::this_thread::sleep_for(std::chrono::milliseconds(200));
          {
            std::lock_guard<std::mutex> lock(mutex);
            std::cout << "task " << i << std::endl;
          }
        });
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "ThreadPool::~ThreadPool()" << std::endl;
  }

  {
    std::cout << "TEST: ThreadPool::abort()" << std::endl;
    ThreadPool pool(4);
    std::mutex mutex;

    for(int i = 0; i < 30; ++i)
    {
      pool.schedule([i, &mutex]{
          std::this_thread::sleep_for(std::chrono::milliseconds(200));
          {
            std::lock_guard<std::mutex> lock(mutex);
            std::cout << "task " << i << std::endl;
          }
        });
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "ThreadPool::abort()" << std::endl;
    pool.abort();
  }

  {
    std::cout << "TEST: empty ThreadPool" << std::endl;
    ThreadPool pool(4);
  }
}

/* EOF */
