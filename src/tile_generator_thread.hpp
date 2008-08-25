/*  $Id$
**   __      __ __             ___        __   __ __   __
**  /  \    /  \__| ____    __| _/_______/  |_|__|  | |  |   ____
**  \   \/\/   /  |/    \  / __ |/  ___/\   __\  |  | |  | _/ __ \
**   \        /|  |   |  \/ /_/ |\___ \  |  | |  |  |_|  |_\  ___/
**    \__/\  / |__|___|  /\____ /____  > |__| |__|____/____/\___  >
**         \/          \/      \/    \/                         \/
**  Copyright (C) 2007 Ingo Ruhnke <grumbel@gmx.de>
**
**  This program is free software; you can redistribute it and/or
**  modify it under the terms of the GNU General Public License
**  as published by the Free Software Foundation; either version 2
**  of the License, or (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
** 
**  You should have received a copy of the GNU General Public License
**  along with this program; if not, write to the Free Software
**  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
**  02111-1307, USA.
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
  Vector2i pos;
  int      scale;
  boost::function<void (TileEntry)> callback;
};

/** Tasks will be grouped into groups, so that one image will be
    handled in one go, instead of accessing multiple files over and
    over again. Groups will be processed in one go. */
struct TileGeneratorMessageGroup { 
  struct TileDescription 
  { 
    boost::function<void (TileEntry)> callback;
  };

  FileEntry entry;
  
  /** [(scale, (x,y,callback)), ...] */
  std::map<int, std::vector<TileDescription> > jobs;
};

class TileGeneratorThread : public Thread
{
private:
  static TileGeneratorThread* current_; 
public:
  static TileGeneratorThread* current() { return current_; }

private:
  bool quit;
  
protected:
  int run();
  
public:
  TileGeneratorThread();
  ~TileGeneratorThread();

  void stop();

  /** Generate tiles for \a file_entry from min_scale to max_scale */
  void request_tiles(const FileEntry& file_entry, int min_scale, int max_scale,
                     const boost::function<void (TileEntry)>& callback);
  
private:
  TileGeneratorThread (const TileGeneratorThread&);
  TileGeneratorThread& operator= (const TileGeneratorThread&);
};

#endif

/* EOF */
