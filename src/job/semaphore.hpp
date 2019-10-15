/*
**  Galapix - an image viewer for large image collections
**  Copyright (C) 2019 Ingo Ruhnke <grumbel@gmail.com>
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

#ifndef HEADER_GALAPIX_JOB_SEMAPHORE_HPP
#define HEADER_GALAPIX_JOB_SEMAPHORE_HPP

#include <mutex>
#include <condition_variable>

class Semaphore
{
public:
  Semaphore(int permits) :
    m_mutex(),
    m_cv(),
    m_permits(permits)
  {}

  void release()
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_permits += 1;
    m_cv.notify_one();
  }

  void aquire()
  {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this]{ return m_permits != 0; });
    m_permits -= 1;
  }

private:
  std::mutex m_mutex;
  std::condition_variable m_cv;
  int m_permits;

private:
  Semaphore(const Semaphore&) = delete;
  Semaphore& operator=(const Semaphore&) = delete;
};

class SemaphoreLock
{
public:
  SemaphoreLock(Semaphore& sem) :
    m_sem(sem)
  {
    m_sem.aquire();
  }

  ~SemaphoreLock()
  {
    m_sem.release();
  }

private:
  Semaphore& m_sem;

private:
  SemaphoreLock(const SemaphoreLock&) = delete;
  SemaphoreLock& operator=(const SemaphoreLock&) = delete;
};

#endif

/* EOF */
