/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2008 Ingo Ruhnke <grumbel@gmx.de>
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

#include "job/thread.hpp"

Thread::Thread()
  : m_thread()
{
}

Thread::~Thread()
{
}

void
Thread::join_thread()
{
  assert(m_thread);
  //assert(thread->joinable());
  
  m_thread->join();

  m_thread.reset();
}

void
Thread::start_thread()
{
  assert(!m_thread);
  m_thread.reset(new boost::thread(boost::bind(&Thread::run_wrap, this)));
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
    assert(!"Thread::run_wrap(): Should never happen");
  }
  catch(...)
  {
    assert(!"Thread::run_wrap(): Unknown exception, should never happen");
  }
}

/* EOF */
