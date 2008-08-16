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

#include <iostream>
#include <boost/bind.hpp>
#include "tile_generator.hpp"
#include "database_thread.hpp"
#include "tile_generator_thread.hpp"
 
TileGeneratorThread* TileGeneratorThread::current_ = 0; 

TileGeneratorThread::TileGeneratorThread()
  : quit(false)
{
  current_ = this;
}

TileGeneratorThread::~TileGeneratorThread()
{
}

void
TileGeneratorThread::request_tiles(int fileid, const std::string& filename)
{
  TileGeneratorMessage msg;

  msg.fileid   = fileid;
  msg.filename = filename;

  msg_queue.push(msg);
}

void
TileGeneratorThread::request_tile(int fileid, const std::string& filename, int x, int y)
{
  // Do some magic to group tile request for the same fileid
}

void
TileGeneratorThread::stop()
{
  quit = true;
}

void
TileGeneratorThread::receive_tile(const Tile& tile)
{
  DatabaseThread::current()->store_tile(tile);
}

int
TileGeneratorThread::run()
{
  quit = false;

  TileGenerator generator;

  while(!quit)
    {
      while(!msg_queue.empty())
        {
          TileGeneratorMessage msg = msg_queue.front();
          msg_queue.pop();

          std::cout << "Generating tiles for: " << msg.filename << std::endl;
          generator.generate_all(msg.fileid, msg.filename,
                                 boost::bind(&TileGeneratorThread::receive_tile, this, _1));
        }
      msg_queue.wait();
    }

  return 0;
}

/* EOF */
