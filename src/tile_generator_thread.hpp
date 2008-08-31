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

#ifndef HEADER_TILE_GENERATOR_THREAD_HPP
#define HEADER_TILE_GENERATOR_THREAD_HPP

#include <boost/function.hpp>
#include <map>
#include "thread.hpp"
#include "thread_message_queue.hpp"
#include "file_entry.hpp"
#include "tile_entry.hpp"

class FileEntry;

/** A Task as received from another thread */
struct TileGeneratorThreadJob
{
  FileEntry entry;
  int       min_scale;
  int       max_scale;
  
  boost::function<void (TileEntry)> callback;
};

class TileGeneratorThread : public Thread
{
private:
  static TileGeneratorThread* current_; 
public:
  static TileGeneratorThread* current() { return current_; }

private:
  bool quit;
  bool working;
  ThreadMessageQueue<TileGeneratorThreadJob> msg_queue;

  int min_scale;
  int max_scale;
  uint32_t fileid;

protected:
  int run();
  
public:
  TileGeneratorThread();
  ~TileGeneratorThread();

  void stop();

  bool is_working() const;

  /** Generate tiles for \a file_entry from min_scale to max_scale */
  void request_tiles(const FileEntry& file_entry, int min_scale, int max_scale,
                     const boost::function<void (TileEntry)>& callback);
  
private:
  void process_message(const TileGeneratorThreadJob& job);

  TileGeneratorThread (const TileGeneratorThread&);
  TileGeneratorThread& operator= (const TileGeneratorThread&);
};

#endif

/* EOF */
