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

#include "job/thread.hpp"

#include <assert.h>
#include <iostream>

Thread::Thread() :
  m_state(kNothing),
  m_thread()
{
}

Thread::~Thread()
{
  assert(m_state == kJoined);
}

void
Thread::join_thread()
{
  assert(m_state == kRunning);
  m_thread.join();
  m_state = kJoined;
}

void
Thread::start_thread()
{
  assert(m_state == kNothing);
  m_thread = std::thread([this]{ run_wrap(); });
  m_state = kRunning;
}

void
Thread::run_wrap()
{
  try
  {
    run();
  }
  catch(std::exception& err)
  {
    std::cout << "Thread: Error: " << err.what() << std::endl;
    assert(false && "Thread::run_wrap(): Should never happen");
  }
  catch(...)
  {
    assert(false && "Thread::run_wrap(): Unknown exception, should never happen");
  }
}

/* EOF */
