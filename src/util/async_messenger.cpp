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

#include "util/async_messenger.hpp"

#include <logmich/log.hpp>

namespace galapix {

AsyncMessenger::AsyncMessenger() :
  m_quit(false),
  m_queue()
{
}

AsyncMessenger::~AsyncMessenger()
{
}

void
AsyncMessenger::run()
{
  while(!m_quit)
  {
    std::function<void()> func;
    while(m_queue.try_pop(func))
    {
      try
      {
        func();
      }
      catch(std::exception& err)
      {
        log_error(err.what());
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void
AsyncMessenger::stop_thread()
{
  m_quit = true;
  m_queue.wakeup();
}

void
AsyncMessenger::queue(const std::function<void ()>& message)
{
  m_queue.wait_and_push(message);
}

} // namespace galapix

/* EOF */
