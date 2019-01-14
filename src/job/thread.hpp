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

#ifndef HEADER_GALAPIX_JOB_THREAD_HPP
#define HEADER_GALAPIX_JOB_THREAD_HPP

#include <thread>

class Thread
{
public:
  Thread();
  virtual ~Thread();

  /** Launch the run() function in a seperate Thread */
  virtual void start_thread();

  virtual void join_thread();

  /** Set conditions so that the run() function can terminate, joining is done in the destructor, not here */
  virtual void stop_thread() =0;

  virtual void run() =0;

private:
  enum { kRunning, kJoined, kNothing } m_state;
  std::thread m_thread;

  void run_wrap();

private:
  Thread (const Thread&);
  Thread& operator= (const Thread&);
};

#endif

/* EOF */
