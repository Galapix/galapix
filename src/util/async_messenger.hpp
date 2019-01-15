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

#ifndef HEADER_GALAPIX_UTIL_ASYNC_MESSENGER_HPP
#define HEADER_GALAPIX_UTIL_ASYNC_MESSENGER_HPP

#include "job/thread.hpp"
#include "job/thread_message_queue2.hpp"

class AsyncMessenger : public Thread
{
public:
  AsyncMessenger();
  ~AsyncMessenger() override;

  void run() override;
  void stop_thread() override;

protected:
  void queue(const std::function<void ()>& message);

private:
  bool m_quit;
  ThreadMessageQueue2<std::function<void ()> > m_queue;

private:
  AsyncMessenger(const AsyncMessenger&);
  AsyncMessenger& operator=(const AsyncMessenger&);
};

#endif

/* EOF */
